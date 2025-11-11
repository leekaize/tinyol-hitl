# TinyOL-HITL

Open-standard streaming k-means for edge devices. Arduino IDE. Multi-platform.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platforms](https://img.shields.io/badge/Platforms-ESP32%20%7C%20RP2350-blue.svg)](core/)
[![Tests](https://github.com/leekaize/tinyol-hitl/actions/workflows/test-core.yml/badge.svg)](https://github.com/YOUR_USERNAME/tinyol-hitl/actions/workflows/test-core.yml)

## Problem

Commercial TinyML locks you in. Inference only. Per-device fees. Closed protocols.

You need: streaming learning, human corrections, open standards, vendor freedom.

## Solution

Single Arduino sketch. Any board (ESP32, RP2350, Arduino, others). Streaming k-means (<100KB RAM). MQTT to open-source SCADA. Human-in-the-loop.

Validate with CWRU dataset and hardware test rig.

## Current Status (Week X/14)

- ✅ Core algorithm: Streaming k-means implemented
- ✅ Platforms: ESP32-S3, RP2350 auto-detect working
- ✅ Dataset streaming: Serial pipeline working (~16 samples/sec)
- ✅ CWRU pipeline: Binary conversion complete (4 fault types, 1904 samples)
- 🚧 Baseline validation: In progress (clustering test)
- 🚧 Sensor integration: ADXL345 wiring complete, code integration pending
- ⏳ HITL corrections: Design complete, MQTT implementation next

## Quick Start

**Install (5 min)**
```bash
# Download Arduino IDE: https://www.arduino.cc/en/software
# File → Preferences → Additional Board Manager URLs:
https://espressif.github.io/arduino-esp32/package_esp32_index.json,https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

# Tools → Board Manager: Install "esp32" and "Raspberry Pi Pico/RP2040/RP2350"
# Tools → Manage Libraries: Install "PubSubClient"
```

**Upload (2 min)**
```bash
# File → Open → core/core.ino
# Tools → Board: Select your board
# Tools → Port: /dev/ttyACM0 (or /dev/ttyUSB0)
# Sketch → Upload
```

**Monitor**
```bash
# Tools → Serial Monitor (115200 baud)
# See: Platform detected, WiFi connected, Model initialized
```

**Stream Dataset**

```bash
cd data/datasets/cwru
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# Download and convert CWRU data
python3 download.py
python3 extract_features.py --input raw/ --output features/
python3 to_binary.py --input features/ --output binary/

# Stream to Arduino
sudo python3 ../../tools/stream_dataset.py /dev/ttyACM0 binary/97_features.bin
```

**Success check:** "Progress: 1904/1904", "Failed ACKs: 0"

## Supported Platforms

| Platform | WiFi | Memory | Speed |
|----------|------|--------|-------|
| ESP32-S3 | ✓ | 520 KB | 240 MHz |
| RP2350 (Pico 2 W) | ✓ | 520 KB | 150 MHz |

## Project Structure
```
core/                # Arduino sketch (ESP32 + RP2350)
├── core.ino         # Main entry point (dataset streaming)
├── config.h         # Platform auto-detect
├── streaming_kmeans.{h,c}  # Core algorithm
├── platform_*.cpp   # Platform-specific WiFi/storage
└── tests/           # Test kmeans and hitl



data/datasets/cwru/   # CWRU bearing dataset
├── download.py       # Fetch .mat files
├── extract_features.py  # Compute RMS/kurtosis/crest/variance
├── to_binary.py      # Convert to Q16.16 binary
├── binary/           # Ready for streaming
└── tools/stream_dataset.py  # Serial streaming (Python)


libraries/           # MQTT connector
integrations/        # supOS-CE, RapidSCADA
hardware/test_rig/   # Motor specs, wiring
tools/power_profiling/ # Current measurement
```

## Why Arduino IDE

Build an open library for community, align with the ease-of-use.

Also pragmatic for quickly deliver research results:
- Single codebase, support MCU of different brands
- Mature WiFi/MQTT libraries
- 5-second upload iteration
- No toolchain setup
- Community support

## Hardware Validation

**Test Rig:** 0.5 HP motor, GY521/ADXL345 sensor, 2 platforms
**Dataset:** CWRU bearing faults (public, reproducible)
**Power:** ESP32 vs RP2350 comparison (future)

## Dataset Streaming

**Current:** Serial @ 115200 baud, ~16 samples/sec
**Format:** 16-byte header + Q16.16 fixed-point samples
**Tested:** 4 fault types (normal, ball, inner, outer)

## Use Cases

**Condition Monitoring:** Bearing faults, motor health, leak detection
**Edge Intelligence:** Continuous learning, no cloud, operator feedback

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design
- [docs/DATASETS.md](docs/DATASETS.md) - CWRU workflow
- [docs/HARDWARE.md](docs/HARDWARE.md) - Test rig (future)
- [docs/TASK_LIST.md](docs/TASK_LIST.md) - Sprint progress

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

**Priority:** <30 days. Ship working code. Perfect later.

## License

Apache-2.0. Industrial-friendly. Fork freely.

---

**Built for:** Embedded ML researchers, industrial IoT engineers
**Validated with:** CWRU dataset, tri-platform comparison, hardware test rig