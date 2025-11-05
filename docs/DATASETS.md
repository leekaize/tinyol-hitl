# Datasets

Public bearing fault datasets for reproducible validation. Stream to MCU. Compare against published baselines.

## Why Public Datasets

**Reproducibility:** Anyone can verify your results using same data.

**Comparison:** Direct apples-to-apples with published papers.

**Development speed:** Test algorithm today. Don't wait for hardware.

**Risk mitigation:** Motor breaks? Stream dataset instead.

## CWRU Bearing Dataset

**Source:** Case Western Reserve University Bearing Data Center

**URL:** https://engineering.case.edu/bearingdatacenter

**Most cited dataset in bearing fault detection literature.** Your results directly comparable.

**Structure:**
- 4 fault types: Ball, Inner race, Outer race, Normal
- 3 severity levels: 0.007", 0.014", 0.021" fault diameter
- 4 load conditions: 0, 1, 2, 3 HP
- Sampling rates: 12 kHz and 48 kHz

**File format:** MATLAB .mat files

**Size:** ~240 files, ~2 GB total

## MFPT Bearing Dataset

**Source:** Machinery Failure Prevention Technology Society

**URL:** https://mfpt.org/fault-data-sets/

**Alternative validation source.** High sampling rate. Different fault scenarios.

**Structure:**
- Outer race, inner race, ball faults
- 3 sensor positions
- Sampling rate: 97,656 Hz
- Baseline: 6 seconds nominal condition

**File format:** MATLAB .mat files

**Size:** ~20 files, ~500 MB total

## Streaming to MCU

**Challenge:** MCU has no filesystem. No MATLAB. No floating-point luxury.

**Solution:** Convert offline. Stream binary. Fixed-point format.

### Conversion Pipeline

**Step 1: Download**
```bash
cd data/datasets/cwru
python3 download.py  # Fetches all files from Case website
```

**Step 2: Extract features**
```bash
python3 extract_features.py --input raw/ --output features/
# RMS, kurtosis, crest factor per 256-sample window
```

**Step 3: Convert to fixed-point binary**
```bash
python3 to_binary.py --input features/ --output binary/
# Q16.16 format, little-endian, 4 bytes per sample
```

**Step 4: Load to MCU**
- SD card: Copy .bin file, read via SPI
- UART: Stream via USB serial at 115200 baud
- Flash: Embed in firmware (only for small datasets)

### Binary Format

**Header (16 bytes):**
```
uint32_t magic;           // 0x4B4D4541 ("KMEA")
uint16_t num_samples;     // Sample count in file
uint8_t  feature_dim;     // Features per sample
uint8_t  fault_type;      // 0=normal, 1=ball, 2=inner, 3=outer
uint32_t sample_rate;     // Original sampling rate (Hz)
uint32_t reserved;        // Future use
```

**Data (num_samples × feature_dim × 4 bytes):**
```
fixed_t sample_0[feature_dim];
fixed_t sample_1[feature_dim];
...
fixed_t sample_N[feature_dim];
```

Fixed-point format: Q16.16 (16 integer bits, 16 fractional bits).

Range: -32768.0 to +32767.99998.

### Feature Extraction

**Time domain (4 features per window):**
- RMS (root mean square): Overall energy
- Kurtosis: Impulsiveness (bearing faults spike)
- Crest factor: Peak-to-RMS ratio
- Variance: Spread

**Window size:** 256 samples at 12 kHz = 21 ms

**Overlap:** 50% (128 samples)

**Why these features:** Proven in bearing fault literature. Low compute. Interpretable.

**Frequency domain (optional, 8 features):**
- FFT magnitude at bearing fault frequencies
- BPFO (ball pass outer race frequency)
- BPFI (ball pass inner race frequency)
- BSF (ball spin frequency)
- FTF (fundamental train frequency)

Trade: 10x compute for 5-10% accuracy gain. Test both.

## Validation Methodology

**Split:** 70% train, 30% test. Stratified by fault type.

**Streaming simulation:**
1. Load binary to MCU flash or SD card
2. Stream samples one-by-one to k-means
3. Every 1000 samples: print confusion matrix
4. Final: accuracy, precision, recall, F1

**Baseline (no HITL):**
- Stream all training samples
- Test on held-out set
- Record accuracy

**With HITL:**
- Stream training samples
- Inject 10% label corrections (simulate expert)
- Test on held-out set
- Measure improvement

**Target:** 15-25% accuracy improvement with HITL vs baseline.

## Papers Using CWRU

Search for these to extract baselines:

- "Bearing fault diagnosis using CNN" (2016) - 99.5% accuracy
- "Deep learning for rotating machinery" (2018) - 98.7% accuracy
- "Edge AI for predictive maintenance" (2021) - 94.2% accuracy on MCU

Your contribution: streaming learning + HITL. Theirs: batch inference.

## Scripts

**data/datasets/cwru/download.py:**
```python
# Fetch all .mat files from CWRU website
# ~2 GB, 10 minutes on fast connection
```

**data/datasets/cwru/extract_features.py:**
```python
# RMS, kurtosis, crest, variance per window
# Input: .mat files
# Output: .csv (one row per window)
```

**data/datasets/cwru/to_binary.py:**
```python
# Convert .csv to Q16.16 binary
# Add header with metadata
# Output: .bin (MCU-compatible)
```

**platforms/rp2350/stream_dataset.c:**
```c
// Read .bin from SD card
// Feed samples to k-means one-by-one
// Print confusion matrix every 1000 samples
```

**platforms/esp32/stream_dataset.c:**
```c
// Same as RP2350 version
// FreeRTOS task reads from SPIFFS
```

## File Locations

```
data/
├── datasets/
│   ├── cwru/
│   │   ├── download.py
│   │   ├── extract_features.py
│   │   ├── to_binary.py
│   │   ├── raw/              # .mat files (gitignored)
│   │   ├── features/         # .csv files (gitignored)
│   │   └── binary/           # .bin files (gitignored)
│   └── mfpt/
│       └── (same structure)
└── streaming/
    ├── README.md
    └── (MCU streaming code lives in platforms/)
```

**.gitignore:** Add raw/, features/, binary/ directories. Too large for repo.

**README in data/:** Instructions for downloading and converting.

## Reproducibility Checklist

- [ ] Download scripts fetch exact files from original sources
- [ ] Feature extraction uses published formulas (cite papers)
- [ ] Binary format documented with byte-level spec
- [ ] Validation split (70/30) seeded for reproducibility
- [ ] Baseline metrics match published papers (±2% acceptable)
- [ ] HITL improvement measured against same baseline
- [ ] Confusion matrices saved for each test run
- [ ] Power consumption measured during dataset streaming

Ship binary conversion scripts first. Test on RP2350. Then ESP32.