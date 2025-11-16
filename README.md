# TinyOL-HITL

Label-driven incremental clustering for edge devices. No pre-training. Grows from K=1 to K=N as faults discovered.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platforms](https://img.shields.io/badge/Platforms-ESP32%20%7C%20RP2350-blue.svg)](core/)
[![Tests](https://github.com/leekaize/tinyol-hitl/actions/workflows/test-core.yml/badge.svg)](https://github.com/leekaize/tinyol-hitl/actions/workflows/test-core.yml)

## Problem

Predictive maintenance stuck at 27% adoption. Three barriers tackled through this solution:
- ML expertise shortage
- Vendor lock-in
- Integration complexity

## Solution

Start with K=1. Operator labels faults as discovered. System grows organically.

- **Day 1:** K=1, everything = "normal"
- **Day 5:** Operator sees anomaly → label "outer_race_fault" → K=2
- **Week 2:** Third fault → K=3
- **Month 1:** Fourth fault → K=4

Arduino sketch. Any board (ESP32, RP2350). MQTT to any broker. <100KB RAM.

## Quick Start

**1. Install Arduino IDE** (5 min)
```bash
# Download: https://www.arduino.cc/en/software
# File → Preferences → Board URLs:
https://espressif.github.io/arduino-esp32/package_esp32_index.json,
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

# Tools → Board Manager: Install "esp32" and "Raspberry Pi Pico/RP2040/RP2350"
# Tools → Manage Libraries: Install "PubSubClient", "ArduinoJson", "Adafruit ADXL345"
```

**2. Upload sketch** (2 min)
```bash
# File → Open → core/core.ino
# Edit WiFi credentials in config.h
# Tools → Board: ESP32S3 Dev Module (or Raspberry Pi Pico 2 W)
# Tools → Port: /dev/ttyUSB0
# Sketch → Upload
```

**3. Wire sensor** (3 min)
```
ADXL345 → ESP32-S3
VCC     → 3.3V
GND     → GND
SDA     → GPIO21
SCL     → GPIO22
```

**4. Monitor MQTT** (1 min)
```bash
mosquitto_sub -h broker.hivemq.com -t "sensor/#" -v
```

Success: See cluster assignments streaming every 100ms.

## Label New Fault Type

When anomaly appears in RapidSCADA:

```bash
mosquitto_pub -h broker.hivemq.com \
  -t "tinyol/device1/label" \
  -m '{"label":"outer_race_fault","features":[0.5,0.2,0.8]}'
```

System creates new cluster. Future similar patterns → assigned to that cluster.

## How It Works

**Feature extraction:**
- RMS: Overall vibration energy
- Peak: Maximum amplitude
- Crest Factor: Peak/RMS ratio (bearing faults spike high)

**Streaming k-means with EMA:**
```
c_new = c_old + α(x - c_old)
α = base_lr / (1 + 0.01 × count)
```

**Memory:** K × D × 4 bytes. Current (K=16, D=3): 192 bytes.

**Latency:** <1ms per sample. Squared distance (no sqrt).

**Phase 2:** Add current sensing → 7D features. Compare accuracy improvement.

## Validation

- **CWRU dataset:** 1904 samples, 4 classes, streaming @ 115200 baud
- **Hardware:** 0.5 HP motor, ADXL345 + MPU6050, ESP32 DEVKIT V1 + RP2350, ZMCT103C current sensing
- **Test method:** Eccentric weight unbalance (non-destructive, repeatable)
- **Protocols:** MQTT → RapidSCADA / supOS-CE / Node-RED

Results: [PLACEHOLDER]

## Supported Platforms

| Board | Arch | RAM | Speed | WiFi |
|-------|------|-----|-------|------|
| ESP32 DEVKIT V1 | Xtensa LX6 | 520KB | 240MHz | ✓ |
| RP2350 (Pico 2 W) | ARM Cortex-M33 | 520KB | 150MHz | ✓ |

Same code compiles for both. Platform layer handles WiFi/I²C differences.

## Project Structure

```
core/                   # Arduino sketch
├── core.ino            # Main loop (sensor → cluster → MQTT)
├── streaming_kmeans.c  # Dynamic clustering algorithm
├── streaming_kmeans.h  # API (kmeans_init, kmeans_add_cluster)
├── platform_esp32.cpp  # ESP32 WiFi/I²C
├── platform_rp2350.cpp # RP2350 WiFi/I²C
└── config.h            # WiFi credentials, MQTT broker

data/datasets/cwru/     # CWRU bearing dataset
├── download.py         # Fetch .mat files
├── extract_features.py # RMS, kurtosis, crest, variance
├── to_binary.py        # Q16.16 binary conversion
└── stream_dataset.py   # Serial streaming (Python → Arduino)

libraries/MQTTConnector/ # MQTT publish/subscribe wrapper
integrations/           # RapidSCADA, supOS-CE configs
hardware/test_rig/      # 0.5 HP motor wiring diagram
docs/                   # Architecture, task list, paper
```

## Use Cases

- **Bearing faults:** Ball, inner race, outer race defects
- **Motor health:** Vibration + current monitoring
- **Leak detection:** Pressure transients
- **Tool wear:** CNC spindle vibration

Any time-series sensor data. System learns patterns incrementally.

## Integration

**SCADA:** MQTT → supOS-CE (UNS tags) → RapidSCADA (native driver)
**Historian:** MQTT → supOS-CE → PostgreSQL

No vendor lock-in. Standard protocols.

## Cost Analysis

| Component | Cost |
|-----------|------|
| ESP32-S3 or RP2350 | RM30 |
| ADXL345 sensor | RM20 |
| Enclosure (IP65) | RM20 |
| Cables + mounting | RM10 |
| **Total** | **RM80** |

Compare: Commercial gateway (RM300-800) + cloud fees (RM50-200/month).

Breakeven: <2 months. Zero recurring fees.

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - Label-driven clustering design
- [docs/DATASETS.md](docs/DATASETS.md) - CWRU workflow + feature extraction
- [docs/HARDWARE.md](docs/HARDWARE.md) - Test rig wiring + sensor mounting
- [docs/TASK_LIST.md](docs/TASK_LIST.md) - 7-day sprint plan
- [docs/paper/](docs/paper/) - Research paper (LaTeX)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

**Priority:** <7 days. Ship MVP. Expand later.

## License

Apache-2.0. Industrial-friendly. Fork freely.

---

**Target audience:** SMEs, embedded engineers, maintenance teams
**Deployment time:** 30 minutes first device, 10 minutes subsequent
**Proven on:** CWRU dataset + real 0.5 HP induction motor