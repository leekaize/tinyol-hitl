# Architecture

System design for dual-platform on-device learning with human corrections.

## Core Algorithm

**Streaming k-means.** Exponential moving average. No batch processing. Update per sample.

**Update rule:**
```
centroid_new = centroid_old + α(sample - centroid_old)
α = base_lr / (1 + 0.01 × count)
```

Learning rate decays as cluster sees more points. Prevents drift from established patterns.

**Memory:** K × D × 4 bytes + metadata. Example: 10 clusters × 32 features = 1.28 KB.

**Precision:** Q16.16 fixed-point. ARM and Xtensa both have fast 32-bit multiply.

**Distance:** Squared Euclidean. No sqrt. Saves 30% compute per sample.

## Platform Requirements

**Memory budget:**
- WiFi stack: 80 KB
- Model (10 clusters × 32 features): 1.3 KB
- Training buffer: 50 KB
- Code: 20 KB
- Margin: 370 KB

**Static allocation only.** No malloc. Deterministic memory. Predictable behavior.

**Cluster count:** Fixed at compile time. Trade flexibility for safety.

## Hardware Abstraction

**Platform API:**
```c
platform_status_t platform_init(model, k, dim, lr);
uint8_t platform_process_point(model, point);
void platform_print_stats(model);
```

Three functions. Core algorithm stays pure. Platform handles I/O, LED, serial.

**ESP32 specifics:**
- FreeRTOS task management
- ESP-IDF component system
- NVS for flash persistence

**RP2350 specifics:**
- Pico SDK bare-metal
- Multicore Arm (unused in v1)
- USB stdio via TinyUSB

## Data Flow

**Inference path:**
```
Sensor → ADC → Feature → K-means → Cluster ID → MQTT → SCADA
```

**Training path:**
```
SCADA → Human Label → MQTT → Update Centroid → Flash → Persist
```

**Flash write trigger:** Every 100 samples or on label correction. Balance persistence vs wear.

## Communication Protocol

**MQTT topics:**
```
sensor/device_id/data          # Streaming features
sensor/device_id/cluster       # Assigned cluster
sensor/device_id/correction    # Human label (subscribe)
sensor/device_id/model         # Centroid updates (publish on change)
```

**QoS 0:** Speed over reliability. Lost sample acceptable. Lost label is not—handled at SCADA layer.

## Integration Architecture

```
MCU → WiFi → MQTT Broker → supOS-CE/RapidSCADA → Ignition/Web HMI
                               ↓
                          PostgreSQL (label history)
                               ↑
                          Analytics (Python)
```

**supOS-CE:** Unified namespace. Tag-based routing. MQTT native.

**RapidSCADA:** Modbus RTU/TCP. OPC-UA. Open-source. Windows/Linux.

**Choice:** Use supOS if pure MQTT. Use RapidSCADA if legacy Modbus infrastructure exists.

## Baseline Comparison

**TinyOL (2021):**
- Memory: 256 KB model
- Method: Batch k-means (100 samples, then update)
- HITL: None
- Platform: Single (STM32)

**Our framework:**
- Memory: <100 KB model
- Method: Streaming (update per sample)
- HITL: Via MQTT corrections
- Platform: Dual (ESP32 + RP2350)

**Target improvement:** 15-25% accuracy with HITL. Measured against CWRU baseline without corrections.

## Performance Targets

**Latency:**
- Sample → Cluster: <50 ms
- Label → Update: <2 s
- WiFi transmit: 1 Hz

**Throughput:**
- RP2350: 150 samples/sec @ 150 MHz
- ESP32: TBD (target 200 samples/sec @ 240 MHz)

**Power:**
- Active: 40-50 mA (WiFi on)
- Sleep: <1 mA (WiFi off, flash retention)
- Battery: 1 year @ 10 Ah (95% sleep, 5% active)

## Memory Layout

**ESP32 (520 KB SRAM):**
```
0x00000 - 0x14000: FreeRTOS kernel + WiFi (80 KB)
0x14000 - 0x15400: K-means model (5 KB)
0x15400 - 0x27000: Training buffer (71 KB)
0x27000 - 0x32000: Application stack (44 KB)
0x32000 - 0x80000: Heap (312 KB margin)
```

**RP2350 (520 KB SRAM):**
```
0x20000000 - 0x20014000: WiFi (CYW43) firmware (80 KB)
0x20014000 - 0x20015400: K-means model (5 KB)
0x20015400 - 0x20027000: Training buffer (71 KB)
0x20027000 - 0x20032000: Stack (44 KB)
0x20032000 - 0x20080000: Unused (312 KB margin)
```

Same layout. Portable code. Different addresses.

## Dataset Integration

**CWRU streaming:**
- Source: 12 kHz accelerometer data
- Format: .mat files → fixed-point binary
- Chunk: 256 samples per packet
- Interface: SD card or UART

**Validation:**
- Hardware: Real motor + sensor (operational)
- Dataset: CWRU/MFPT (reproducibility)
- Metrics: Confusion matrix, accuracy, F1 score

## Algorithm Decisions

**Why streaming k-means?**
- Simplest unsupervised algorithm (200 lines C)
- Lowest memory (linear in K and D)
- Proven convergence (Bottou 1998)

**Why not neural networks?**
- Backprop memory overhead (gradients)
- Convergence requires batch
- Harder to explain to operators

**Why not GMM?**
- Covariance matrices: O(D²) memory
- EM algorithm needs batch iteration
- Computational cost too high

**Why fixed-point?**
- No FPU on Cortex-M33 (RP2350)
- ESP32 has FPU, but fixed-point is portable
- 16.16 format sufficient for condition monitoring

## Open Questions

**Feature extraction:** Time domain (RMS, kurtosis) or frequency domain (FFT)? Test both. Measure accuracy vs compute.

**Label conflicts:** Human says A, model says B. Strategy: trust human, decay old centroid slowly (α = 0.05).

**Flash wear:** 100k write cycles typical. At 100 samples/write, that's 10M samples. Acceptable for research.

## Future Extensions

**Neural augmentation:** Add 1-layer perceptron after k-means. Optional compilation flag.

**Multi-sensor fusion:** Vibration + temperature + current. Extend feature dimension.

**OTA updates:** Flash new model via MQTT. Bootloader support needed.

**Energy harvesting:** Solar or vibration. Battery becomes backup, not primary.