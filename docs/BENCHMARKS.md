# Benchmark Results

**Status:** Data collection in progress

## Literature Baselines (CWRU Dataset)

| Method | Accuracy | Memory | Source |
|--------|----------|--------|--------|
| CNN (traditional) | 95-98% | ~500KB | Zhang 2016 |
| Lite CNN | 99.86% | 153K params | Yoo & Baek 2023 |
| SVM | 85-95% | Cloud | Various |
| K-means (batch) | ~80% | O(nK) | Amruthnath 2018 |

**TO-DO:** Verify exact citations, add page numbers.

---

## TinyOL-HITL Results

### Experiment 1: CWRU Streaming (K=1 → K=4)

| Metric | Value | Notes |
|--------|-------|-------|
| Total samples | [TODO] | 4 classes |
| Baseline accuracy (K=1) | [TODO]% | Everything = "normal" |
| After 1 label (K=2) | [TODO]% | Ball fault separated |
| After 4 labels (K=4) | [TODO]% | All classes |
| + 10% corrections | [TODO]% | HITL improvement |

**Expected range:** 70-85% unsupervised, +15-25% with HITL

### Experiment 2: Real Motor

| Condition | Samples | Cluster | Confidence |
|-----------|---------|---------|------------|
| Baseline (healthy) | [TODO] | 0 | [TODO]% |
| Eccentric 50 g·mm | [TODO] | 1 | [TODO]% |
| Eccentric 200 g·mm | [TODO] | 2 | [TODO]% |

### Experiment 3: Cross-Platform Consistency

| MCU | Cluster Assignments | Delta from ESP32 |
|-----|---------------------|------------------|
| ESP32 DEVKIT V1 | [baseline] | 0 |
| RP2350 Pico 2W | [TODO] | [TODO] |

**Target:** Delta < 0.1 (fixed-point consistency)

---

## Memory Measurements

```
Component           ESP32       RP2350
-----------         -----       ------
Model struct        [TODO] B    [TODO] B
Peak heap           [TODO] B    [TODO] B
Flash usage         [TODO] KB   [TODO] KB
```

**Method:** Arduino IDE "Sketch uses X bytes" output

---

## Latency Measurements

| Operation | Time (μs) | Method |
|-----------|-----------|--------|
| Feature extraction | [TODO] | micros() delta |
| kmeans_update() | [TODO] | micros() delta |
| MQTT publish | [TODO] | micros() delta |
| Total loop | [TODO] | micros() delta |

**Target:** < 20ms total (10Hz sampling)

---

## Data Collection Commands

```bash
# CWRU streaming
python3 tools/stream_dataset.py /dev/ttyUSB0 binary/normal.bin --verbose

# Capture Serial output
screen -L /dev/ttyUSB0 115200
# Log saved to screenlog.0

# Parse results
grep "Cluster" screenlog.0 | cut -d':' -f2 > clusters.txt
```

---

## Confusion Matrix Template

```
              Predicted
              N    B    I    O
Actual  N   [  ] [  ] [  ] [  ]
        B   [  ] [  ] [  ] [  ]
        I   [  ] [  ] [  ] [  ]
        O   [  ] [  ] [  ] [  ]

N=Normal, B=Ball, I=Inner race, O=Outer race
```

---

## How to Interpret

**Unsupervised baseline (K=1):**
- Expected ~25% (random chance for 4 classes)
- Shows system initializes correctly

**After labeling (K=4):**
- Target: >70% for publication
- Literature comparison: Batch k-means ~80%

**With HITL corrections:**
- Target: +15-25% improvement
- Shows operator feedback value

---

## If Experiments Fail

Honest reporting options:

1. **Partial data:** Report what worked, explain blockers
2. **Simulation:** Use synthetic data with known clusters
3. **Literature estimates:** "Expected X% based on similar work in [cite]"

Never fabricate numbers. Mark estimates clearly.