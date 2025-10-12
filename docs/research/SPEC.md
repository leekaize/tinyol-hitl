# Technical Specification

**Version:** 0.1.0 | **Updated:** 2025-10-12

---

## System Overview

Adaptive TinyML with human-in-the-loop on 520KB SRAM edge devices. No cloud. Streaming learning. Flash persistence.

---

## Hardware: RP2350

- Dual Cortex-M33 @ 150MHz
- 520KB SRAM, 16MB flash
- CYW43439 WiFi (2.4GHz)
- Board: Pico 2 WH

---

## Memory Budget

| Component | Allocation |
|-----------|-----------|
| WiFi stack | 80KB |
| Model (5 clusters × 10 features) | 100KB |
| Training buffer | 50KB |
| Code | 20KB |
| Margin | 270KB |

**Rule:** Static allocation only. No malloc.

---

## Algorithm: Online K-Means

**Update rule:** `μ_k(t+1) = 0.9 × μ_k(t) + 0.1 × x(t)`

**Why:** Simplest streaming unsupervised. O(k×d) memory. Interpretable.

**Rejected alternatives:**
- Neural nets: Backprop overhead
- GMM: Computational cost
- DBSCAN: Not streaming

---

## Related Work & Baselines

### State-of-Art Edge Learning

**TinyML Foundation:**
- TensorFlow Lite Micro (Google) - Inference only, no training
- Edge Impulse - Cloud preprocessing required
- Neurala - Commercial, closed-source

**Gap:** No open framework for on-device incremental learning with HITL.

### Online Learning on MCUs

**Key papers to review:**
1. "TinyOL: On-device Learning on IoT Devices" (2021) - 256KB SRAM baseline
2. "Streaming k-means on resource-constrained devices" (2019)
3. "Human-in-the-loop active learning for edge systems" (2022)

**Baseline metrics to beat:**
- Memory: <200KB model
- Accuracy: 15-25% improvement with HITL
- Latency: <2s label-to-update

**Literature search TODO:** Conduct systematic review Days 4-5.

---

## Architecture

```
Sensor → Features → Clustering → WiFi → supOS-CE → Ignition → Labels → Update
```

**Integration:**
- MQTT to supOS-CE UNS (industrial standard)
- Ignition for visualization (HMI layer)
- QoS 0 (latency > reliability)

---

## Performance Requirements

| Metric | Target |
|--------|--------|
| Sample → Cluster | <50ms |
| Label → Update | <2s |
| WiFi transmit | 1Hz |
| Battery life | 1yr @ 10000mAh (low-power mode dominant) |

---

## Open Questions

1. **Feature extraction:** Time vs frequency domain? (Decide Day 3)
2. **Label conflicts:** How to resolve? (Decide Day 5)
3. **Baseline comparison:** Which TinyML system? (Literature Day 4-5)

---

## Research Contribution

**Novel aspects:**
- Open-source framework (vs commercial closed)
- On-device training (vs inference-only)
- Industrial integration (supOS/Ignition)

**Incremental improvements:**
- Memory optimization vs TinyOL baseline
- Streaming k-means vs batch methods
- Flash persistence strategy

---

## References

- [RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
- TinyML literature review (to be conducted Day 4-5)