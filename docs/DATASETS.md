# Datasets

Public bearing fault datasets for reproducible validation. Arduino IDE workflow. Stream to any platform.

## Why Public Datasets

**Reproducibility:** Anyone verifies your results with same data.

**Comparison:** Direct benchmark against published papers.

**Development speed:** Test algorithm today. Hardware waits.

**Risk mitigation:** Motor breaks? Dataset keeps you moving.

## CWRU Bearing Dataset

**Source:** Case Western Reserve University Bearing Data Center

**URL:** https://engineering.case.edu/bearingdatacenter

**Most cited dataset in bearing fault literature.** Your results directly comparable.

**Structure:**
- 4 fault types: Ball, Inner race, Outer race, Normal
- 3 severity levels: 0.007", 0.014", 0.021" fault diameter
- 4 load conditions: 0, 1, 2, 3 HP
- Sampling rates: 12 kHz and 48 kHz

**File format:** MATLAB .mat files

**Size:** ~240 files, ~2 GB total

## Conversion Pipeline

**Goal:** .mat files → Fixed-point binary → Arduino sketch

### Step 1: Download
```bash
cd data/datasets/cwru
python3 download.py  # Fetches from Case website
```

### Step 2: Extract Features
```bash
python3 extract_features.py --input raw/ --output features/
# RMS, kurtosis, crest factor, variance per 256-sample window
```

### Step 3: Convert to Binary
```bash
python3 to_binary.py --input features/ --output binary/
# Q16.16 format, MCU-compatible
```

### Step 4: Integration Methods

**Option A: SD Card (Recommended for testing)**
```cpp
// In core.ino after setup()
#include <SD.h>
File dataFile = SD.open("bearing_fault.bin");
// Read header, stream samples to kmeans_update()
```

**Option B: Serial Streaming**
```cpp
// Stream from PC via Serial
// Python script sends binary, Arduino reads
while (Serial.available() >= 4) {
  fixed_t sample = readFixedFromSerial();
  kmeans_update(&model, &sample);
}
```

**Option C: Flash Embedding (Small datasets only)**
```cpp
// Add to core.ino
const PROGMEM fixed_t dataset[] = {/* binary data */};
// Loop through array, feed to kmeans_update()
```

## Streaming to MCU (MVP: Serial Approach)

**Decision:** Serial streaming chosen for MVP (no hardware dependencies).

**PC-side Python script** (`tools/stream_dataset.py`):
```python
import serial
import struct

def stream_binary(port, binary_file):
    ser = serial.Serial(port, 115200)
    with open(binary_file, 'rb') as f:
        header = f.read(16)
        ser.write(header)

        while chunk := f.read(256):  # 64 samples × 4 bytes
            ser.write(chunk)
            ack = ser.read(1)  # Wait for MCU ready
```

**MCU-side Arduino code** (`core/core.ino`):
```cpp
void stream_from_serial() {
  if (Serial.available() >= sizeof(fixed_t) * model.feature_dim) {
    fixed_t sample[MAX_FEATURES];
    Serial.readBytes((char*)sample, sizeof(sample));
    uint8_t cluster = kmeans_update(&model, sample);
    Serial.write(0x06);  // ACK byte
  }
}
```

**Why Serial:**
- No SD card wiring required
- Fast iteration (10 seconds to restart test)
- Works on all platforms (USB-CDC)
- Fallback: Embed small dataset in Flash later if needed

## Binary Format Specification

### Header (16 bytes)
```c
struct dataset_header {
    uint32_t magic;         // 0x4B4D4541 ("KMEA")
    uint16_t num_samples;   // Sample count
    uint8_t  feature_dim;   // Features per sample
    uint8_t  fault_type;    // 0=normal, 1=ball, 2=inner, 3=outer
    uint32_t sample_rate;   // Original rate (Hz)
    uint32_t reserved;      // Future use
};
```

### Data Section
```c
fixed_t samples[num_samples][feature_dim];  // Q16.16 format
```

**Q16.16 fixed-point:**
- 16 integer bits, 16 fractional bits
- Range: -32768.0 to +32767.99998
- Conversion: `int32_t = float * 65536`

## Feature Extraction Details

**Time domain (4 features per 256-sample window):**

**RMS:** `sqrt(mean(x²))` - Overall energy

**Kurtosis:** `E[(x - μ)⁴] / σ⁴` - Impulsiveness

**Crest Factor:** `max(|x|) / RMS` - Peak ratio

**Variance:** `E[(x - μ)²]` - Spread

**Window:** 256 samples @ 12 kHz = 21 ms

**Overlap:** 50% (128 samples)

**Why these:** Low compute. Proven in literature. Interpretable.

## Arduino IDE Integration

### Example Sketch Extension
```cpp
// Add to core.ino for dataset streaming

void streamDataset() {
  // Read from SD or Serial
  fixed_t point[FEATURE_DIM];

  while (readNextSample(point)) {
    uint8_t cluster = kmeans_update(&model, point);

    Serial.printf("Sample %lu -> Cluster %d\n",
                  model.total_points, cluster);

    // Print stats every 100 samples
    if (model.total_points % 100 == 0) {
      printConfusionMatrix();
    }
  }
}
```

## Validation Methodology

**Split:** 70% train, 30% test. Stratified by fault type.

**Baseline (no HITL):**
1. Stream training samples
2. Test on held-out set
3. Record accuracy

**With HITL:**
1. Stream training samples
2. Inject 10% corrections (simulated expert)
3. Test on held-out set
4. Measure improvement

**Target:** 15-25% accuracy gain with HITL vs baseline.

## Published Baselines (CWRU)

Search these for comparison:
- "CNN for bearing faults" (2016): 99.5% accuracy
- "Deep learning rotating machinery" (2018): 98.7% accuracy
- "Edge AI predictive maintenance" (2021): 94.2% on MCU

**Your contribution:** Streaming + HITL. Theirs: Batch inference.

## File Organization
```
data/datasets/cwru/
├── download.py          # Fetch from Case website
├── extract_features.py  # Compute features
├── to_binary.py         # Convert to MCU format
├── raw/                 # .mat files (gitignored)
├── features/            # .csv files (gitignored)
└── binary/              # .bin files (gitignored)
```

## Platform-Specific Notes

**ESP32-S3:**
- Use SD.h library for SD card access
- WiFi streams results during testing
- 520 KB RAM handles large buffers

**RP2350:**
- LittleFS for flash storage
- SD card via SPI
- Similar memory to ESP32

## Quick Start: Test on Hardware

**1. Prepare Dataset**
```bash
cd data/datasets/cwru
python3 download.py
python3 extract_features.py --input raw/ --output features/
python3 to_binary.py --input features/ --output binary/
```

**2. Copy to SD Card**
```bash
cp binary/normal_1000_01.bin /media/SD_CARD/
```

**3. Modify core.ino**
```cpp
// Add SD reading logic after setup()
// Stream samples to kmeans_update()
```

**4. Upload and Monitor**
```
Tools → Serial Monitor (115200 baud)
Watch clustering in real-time
```

## Reproducibility Checklist

- [ ] Download scripts fetch exact files
- [ ] Feature formulas documented (cite papers)
- [ ] Binary format byte-level spec complete
- [ ] Validation split seeded (reproducible)
- [ ] Baseline metrics match literature (±2%)
- [ ] HITL improvement measured vs baseline
- [ ] Confusion matrices saved per run

## Next Steps for MVP

1. **Test conversion pipeline** - Verify one .mat file end-to-end
2. **SD card integration** - Add SD reading to core.ino
3. **Serial monitor validation** - Watch clustering happen
4. **Confusion matrix** - Add tracking to sketch
5. **Hardware test** - Compare dataset vs real motor

**Priority:** Get one dataset file streaming through Arduino sketch. Perfect later.

## Advanced: MFPT Dataset

**Source:** Machinery Failure Prevention Technology Society

**URL:** https://mfpt.org/fault-data-sets/

**Structure:** 97,656 Hz sampling, 3 sensor positions

**Status:** Pipeline TBD. CWRU sufficient for MVP.

Ship CWRU integration first. Expand coverage later.