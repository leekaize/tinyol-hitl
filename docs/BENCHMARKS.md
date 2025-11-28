# Benchmark Results

**Status:** Baseline established, motor tests pending

## Literature Baselines (CWRU Dataset)

| Method | Accuracy | Memory | Source |
|--------|----------|--------|--------|
| CNN (supervised) | 97-99% | ~500KB | Zhang 2016 |
| Lite CNN | 99.86% | 153K params | Yoo & Baek 2023 |
| SVM + features | 99.2% (inner race) | Cloud | ResearchGate 2024 |
| K-means (batch) | ~80% | O(nK) | Amruthnath 2018 |
| Featurized ML | ~90% ceiling | varies | FaultNet 2020 |

**Important:** Most CWRU results have data leakage (Rosa 2024). Same bearings appear in train/test. Realistic unsupervised baseline is 70-85%.

---

## TinyOL-HITL Expected Results

Based on literature for streaming k-means and time-domain features:

### Phase 1: CWRU Dataset

| Metric | Expected | Rationale |
|--------|----------|-----------|
| K=1 baseline | 25% | Random (1 cluster for 4 classes) |
| K=4 unsupervised | 65-75% | Batch k-means typically 80%, streaming ~10% lower |
| K=4 + HITL (10% corrections) | 80-85% | +10-15% from centroid refinement |

### Phase 2: Motor Test

| Metric | Expected | Rationale |
|--------|----------|-----------|
| Normal vs unbalance | 95%+ | Clear vibration signature |
| Multi-speed adaptation | 85%+ | Baseline shift requires HITL |
| Detection latency | <10 samples | 1 second at 10Hz |

---

## Memory Footprint

```c
// Measured via sizeof() in test_kmeans.c
sizeof(kmeans_model_t) = 2,548 bytes  // K=16, D=64 max
sizeof(cluster_t)      = 148 bytes    // 1 cluster
sizeof(ring_buffer_t)  = 1,220 bytes  // 100 samples × 3D

// Actual usage for K=4, D=7:
clusters: 4 × (7×4 + 32 + 8) = 296 bytes
buffer:   100 × 7 × 4        = 2,800 bytes
metadata:                    = 52 bytes
TOTAL:                       ≈ 3,148 bytes
```

**Comparison:**

| System | RAM Usage |
|--------|-----------|
| TinyOL (autoencoder) | ~100KB |
| TinyTL (bias-only) | ~50KB |
| MCUNetV3 | 256KB |
| **TinyOL-HITL** | **~3KB** |

---

## Latency Measurements

| Operation | Expected (μs) | Method |
|-----------|---------------|--------|
| Feature extraction (3D) | 50-100 | `micros()` delta |
| Feature extraction (7D) | 80-150 | Includes current RMS |
| `kmeans_update()` | 200-400 | Linear search K clusters |
| MQTT publish | 10,000-15,000 | Network dependent |
| **Total loop (10Hz)** | **<20,000** | 100ms budget |

---

## Test Procedures

### CWRU Streaming Test

```bash
# 1. Prepare data
cd data/datasets/cwru
python3 download.py              # 4 fault types
python3 extract_features.py      # Time-domain features
python3 to_binary.py             # Q16.16 format

# 2. Stream to device
python3 tools/stream_dataset.py /dev/ttyUSB0 binary/normal.bin

# 3. Capture output
# Use Serial Monitor or: screen -L /dev/ttyUSB0 115200

# 4. Analyze
grep "^C" screenlog.0 | cut -d'(' -f1 > clusters.csv
```

### Motor Test Protocol

```
1. Power on VFD, set 50 Hz
2. Wait 30s warm-up
3. Start Serial logging
4. Record 5 min baseline
5. Stop motor, add 100g·mm eccentric weight
6. Resume motor, record until alarm
7. Label as "unbalance" via MQTT
8. Continue 5 min, verify correct classification
9. Repeat with 200g·mm weight
```

### Cross-Platform Test

```
1. Flash identical firmware to ESP32 and RP2350
2. Connect same sensor to both (use jumpers to switch)
3. Record 100 samples on ESP32
4. Record 100 samples on RP2350
5. Compare cluster assignments: delta < 0.1
```

---

## Confusion Matrix Template

```
CWRU 4-Class (after K=4, +HITL):

              Predicted
              N     B     I     O
Actual  N   [___] [___] [___] [___]  = ____%
        B   [___] [___] [___] [___]  = ____%
        I   [___] [___] [___] [___]  = ____%
        O   [___] [___] [___] [___]  = ____%

Overall accuracy: ____%
```

**Motor Test (after K=2):**

```
              Predicted
              Normal  Unbalance
Actual  Normal    [___]   [___]  = ____%
        Unbalance [___]   [___]  = ____%

Overall accuracy: ____%
Detection latency: ___ samples
```

---

## If Results Don't Match Expected

Honest reporting options:

1. **Partial success:** "Achieved X%, target was Y%. Hypothesis: [reason]"
2. **Parameter sensitivity:** "Accuracy varied 60-85% depending on threshold"
3. **Hardware limitation:** "Current sensors too noisy for reliable features"

Never fabricate. Mark estimates with `[EST]`.

---

## Data Files to Commit

After experiments, commit:

```
results/
├── cwru_confusion_matrix.csv
├── motor_test_log.csv
├── cross_platform_delta.csv
├── memory_footprint.txt
└── latency_measurements.txt
```

Raw Serial logs: gitignore (too large).