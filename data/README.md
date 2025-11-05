# Dataset Integration

Public bearing fault datasets for reproducible validation.

## CWRU Dataset

Case Western Reserve University bearing data.

**Download:**
```bash
cd data/datasets/cwru
python3 download.py        # ~2 GB, 240 files
python3 extract_features.py --input raw/ --output features/
python3 to_binary.py --input features/ --output binary/
```

**Binary format:** 16-byte header + Q16.16 samples
**Features:** RMS, kurtosis, crest factor, variance

See `datasets/cwru/README.md` for specification.

## Streaming to MCU

Load .bin files via:
- SD card (SPI interface)
- Serial (UART @ 115200 baud)
- Flash (embed in firmware, small datasets only)