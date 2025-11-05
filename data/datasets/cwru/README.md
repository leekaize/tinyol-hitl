# CWRU Bearing Dataset

Case Western Reserve University bearing fault dataset. Most cited in literature.

## Source

**URL:** https://engineering.case.edu/bearingdatacenter
**Free:** Yes (academic/research use)
**Size:** ~2 GB, 240 files

## Structure

**Fault types:** Ball, Inner race, Outer race, Normal
**Severity levels:** 0.007", 0.014", 0.021" fault diameter
**Load conditions:** 0, 1, 2, 3 HP
**Sampling rates:** 12 kHz and 48 kHz

## File Format

**Original:** MATLAB .mat files
**Converted:** Binary .bin files (MCU-compatible)

## Binary Format Specification

### Header (16 bytes)
```c
struct dataset_header {
    uint32_t magic;         // 0x4B4D4541 ("KMEA")
    uint16_t num_samples;   // Sample count in file
    uint8_t  feature_dim;   // Features per sample
    uint8_t  fault_type;    // 0=normal, 1=ball, 2=inner, 3=outer
    uint32_t sample_rate;   // Original sampling rate (Hz)
    uint32_t reserved;      // Future use
};
```

### Data (num_samples × feature_dim × 4 bytes)
```c
fixed_t samples[num_samples][feature_dim];  // Q16.16 format
```

**Q16.16 fixed-point:**
- 16 integer bits, 16 fractional bits
- Range: -32768.0 to +32767.99998
- Conversion: `int32_t = float * 65536`

### Example
```
Offset  Size  Value       Description
------  ----  ----------  -----------
0x00    4     0x4B4D4541  Magic ("KMEA")
0x04    2     1000        Number of samples
0x06    1     4           Features per sample
0x07    1     2           Fault type (inner race)
0x08    4     12000       Sample rate (12 kHz)
0x0C    4     0           Reserved
0x10    16    ...         Sample 0 (4 features × 4 bytes)
0x20    16    ...         Sample 1
...
```

## Feature Extraction

### Time Domain (4 features per window)

**RMS (Root Mean Square):** Overall energy
```
RMS = sqrt(mean(x²))
```

**Kurtosis:** Impulsiveness (bearing faults spike)
```
Kurtosis = E[(x - μ)⁴] / σ⁴
```

**Crest Factor:** Peak-to-RMS ratio
```
Crest = max(|x|) / RMS
```

**Variance:** Spread
```
Variance = E[(x - μ)²]
```

**Window size:** 256 samples at 12 kHz = 21 ms
**Overlap:** 50% (128 samples)

## Conversion Pipeline

### Step 1: Download
```bash
python3 download.py
# Fetches all .mat files from Case website
# Output: raw/ directory (~2 GB)
```

### Step 2: Extract Features
```bash
python3 extract_features.py --input raw/ --output features/
# Computes RMS, kurtosis, crest, variance per window
# Output: features/*.csv (one row per window)
```

### Step 3: Convert to Binary
```bash
python3 to_binary.py --input features/ --output binary/
# Converts to Q16.16 format with header
# Output: binary/*.bin (MCU-compatible)
```

## Validation Methodology

### Data Split
**Training:** 70% (stratified by fault type)
**Testing:** 30% (held-out set)

### Baseline (No HITL)
1. Stream all training samples
2. Test on held-out set
3. Record accuracy

### With HITL
1. Stream training samples
2. Inject 10% label corrections (simulate expert)
3. Test on held-out set
4. Measure improvement

**Target:** 15-25% accuracy improvement with HITL vs baseline.

## Baseline Papers

Search these for published baselines:
- "Bearing fault diagnosis using CNN" (2016) - 99.5% accuracy
- "Deep learning for rotating machinery" (2018) - 98.7% accuracy
- "Edge AI for predictive maintenance" (2021) - 94.2% accuracy on MCU

**Your contribution:** Streaming learning + HITL. Theirs: Batch inference.

## Labels
```c
enum fault_type {
    NORMAL = 0,
    BALL = 1,
    INNER = 2,
    OUTER = 3
};
```

## Streaming to MCU

### SD Card
```c
#include <SD.h>
File file = SD.open("bearing_fault.bin");
file.read(&header, sizeof(header));
for (uint16_t i = 0; i < header.num_samples; i++) {
    file.read(sample, header.feature_dim * sizeof(fixed_t));
    kmeans_update(&model, sample);
}
```

### Serial
```python
import serial
ser = serial.Serial('/dev/ttyUSB0', 115200)
with open('bearing_fault.bin', 'rb') as f:
    ser.write(f.read())
```

## File Locations
```
data/datasets/cwru/
├── download.py          # Fetch from Case website
├── extract_features.py  # Compute time-domain features
├── to_binary.py         # Convert to MCU format
├── raw/                 # .mat files (gitignored)
├── features/            # .csv files (gitignored)
└── binary/              # .bin files (gitignored)
```

**.gitignore:** `raw/`, `features/`, `binary/` (too large for repo)