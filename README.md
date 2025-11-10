# TinyOL-HITL

Open-standard streaming k-means for edge devices. Arduino IDE. Multi-platform.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platforms](https://img.shields.io/badge/Platforms-ESP32%20%7C%20RP2350-blue.svg)](core/)

## Problem

Commercial TinyML locks you in. Inference only. Per-device fees. Closed protocols.

You need: streaming learning, human corrections, open standards, vendor freedom.

## Solution

Single Arduino sketch. Any board (ESP32, RP2350, Arduino, others). Streaming k-means (<100KB RAM). MQTT to open-source SCADA. Human-in-the-loop.

Validate with CWRU dataset and hardware test rig.

## Current Status (Week X/14)

- ✅ Core algorithm: Streaming k-means implemented
- ✅ Platforms: ESP32-S3, RP2350 auto-detect working
- 🚧 Dataset streaming: In progress (Serial approach)
- 🚧 Sensor integration: ADXL345 wiring complete, code integration pending
- ⏳ HITL corrections: Design complete, MQTT implementation next
- ⏳ CWRU validation: Pipeline ready, awaiting sensor data

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
# Tools → Port: Select USB port
# Sketch → Upload
```

**Monitor**
```bash
# Tools → Serial Monitor (115200 baud)
# See: Platform detected, WiFi connected, Model initialized
```

## Supported Platforms

| Platform | WiFi | Memory | Speed | Use Case |
|----------|------|--------|-------|----------|
| ESP32-S3 | ✓ | 520 KB | 240 MHz | Production (WiFi + MQTT) |
| RP2350 (Pico 2 W) | ✓ | 520 KB | 150 MHz | Power comparison |

## Project Structure
```
core/                # Arduino sketch (ESP32 + RP2350)
libraries/           # MQTT connector
integrations/        # supOS-CE, RapidSCADA
data/datasets/cwru/  # Download, convert tools
hardware/test_rig/   # Motor specs, wiring
tools/power_profiling/ # Current measurement
platforms/rp2350/    # Native SDK reference (deprecated)
```

## Why Arduino IDE

Pragmatic for <30 days:
- Single codebase, support MCU of different brands
- Mature WiFi/MQTT libraries
- 5-second upload iteration
- No toolchain setup
- Community support

## Hardware Validation

**Test Rig:** 0.5 HP motor, ADXL345 sensor, 3 platforms
**Dataset:** CWRU bearing faults (public, reproducible)
**Power:** ESP32 vs RP2350 comparison

## Use Cases

**Condition Monitoring:** Bearing faults, motor health, leak detection
**Edge Intelligence:** Continuous learning, no cloud, operator feedback

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System design
- [docs/DATASETS.md](docs/DATASETS.md) - CWRU methodology
- [docs/HARDWARE.md](docs/HARDWARE.md) - Test rig setup
- [docs/POWER.md](docs/POWER.md) - Profiling guide
- [hardware/test_rig/](hardware/test_rig/) - Wiring diagrams
- [integrations/](integrations/) - SCADA integration

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

**Priority:** <30 days. Ship working code. Perfect later.

## License

Apache-2.0. Industrial-friendly. Fork freely.

---

**Built for:** Embedded ML researchers, industrial IoT engineers
**Validated with:** CWRU dataset, tri-platform comparison, hardware test rig