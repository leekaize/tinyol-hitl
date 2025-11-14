# 7-Day Sprint to Paper Submission

## Status: Algorithm ready, integration needed

## Day 1: Wire sensor → MQTT → RapidSCADA

**Goal:** Sensor data visible in SCADA

- [ ] Add sensor to core.ino (GY521, ADXL345 & ZMCT103C, allow user selection on connected sensors)
- [ ] Add MQTT publish (already have MQTTConnector library)
- [ ] Install RapidSCADA, add MQTT driver
- [ ] Configure subscription: `sensor/#`
- [ ] Create table view showing cluster ID + timestamp

**Success check:** RapidSCADA table updates every 100ms with live cluster assignments

**Fallback:** If RapidSCADA fails, just use mosquitto_sub terminal output

## Day 2: CWRU baseline validation

**Goal:** Prove algorithm works on public dataset

- [ ] Stream CWRU dataset via Serial (use existing stream_dataset.py)
- [ ] Log cluster assignments per sample
- [ ] Compute confusion matrix (4 classes)
- [ ] Compare to literature baseline (SVM: 85-95%)

**Success check:** Unsupervised accuracy >70%, matches SVM range

**Data:** 1904 samples, 4 classes (normal, ball, inner, outer)

## Day 3: Real motor baseline

**Goal:** Show it works on hardware

- [ ] Wire ADXL345 + MPU6050 to motor housing
- [ ] Wire ZMCT103C to motor phases (L1, L2, L3)
- [ ] Run motor 5 min @ 1500 RPM (no weights = healthy)
- [ ] Mount pulley, test eccentric weight attachment
- [ ] Log baseline: 300 samples, verify tight clustering

**Success check:** Cluster radius <0.3 units, >95% samples in single cluster

**Fallback:** If motor unavailable, synthetic vibration data (function generator + speaker)

## Day 4: Label-driven growth demo

**Goal:** Prove dynamic clustering

**Scenario 1 - Add second cluster:**
- [ ] Add mild unbalance (50 g·mm)
- [ ] Run 2 min, see vibration increase in SCADA
- [ ] Label: `{"label":"mild_unbalance","features":[x,y,z,rms,i1,i2,i3]}`
- [ ] Verify K=2, new cluster forms

**Scenario 2 - Add third cluster:**
- [ ] Change speed to 1800 RPM (or different load)
- [ ] Label: `{"label":"high_speed_normal","features":[x,y,z]}`
- [ ] Verify K=3

**Success check:** System grows 1→2→3 clusters, separation visible in SCADA

## Day 5: HITL correction validation

**Goal:** Show corrections improve accuracy

- [ ] Inject 5 intentional misclassifications (assign wrong cluster via correction message)
- [ ] Re-run CWRU stream
- [ ] Measure accuracy before vs after corrections
- [ ] Target: +10-15% improvement

**Correction format:**
```json
{"point": [0.5,0.2,0.8], "old_cluster": 2, "new_cluster": 1}
```

**Success check:** Accuracy improves measurably

## Day 6: Cross-platform validation

**Goal:** Prove portability (Xtensa + ARM)

- [ ] Upload identical sketch to RP2350 (Pico 2 W)
- [ ] Stream same CWRU dataset
- [ ] Compare cluster assignments: ESP32 vs RP2350
- [ ] Measure delta: max(|c_ESP32 - c_RP2350|)

**Success check:** Delta <0.1 (fixed-point consistency)

**If RP2350 unavailable:** Document that code compiles for both, defer actual test

## Day 7: Paper + documentation finalization

**Goal:** Submission-ready paper

**Morning:**
- [ ] Update paper Section V (Results) with actual numbers
- [ ] Generate figures: confusion matrices, cluster visualization
- [ ] Fill placeholders: [PLACEHOLDER: X%] → actual accuracy
- [ ] Compile PDF, verify citations

**Afternoon:**
- [ ] Record 3-min demo video:
  - Upload sketch
  - Stream CWRU data
  - Add label via MQTT
  - Show cluster growth
- [ ] Push to GitHub with tag `v1.0-paper`
- [ ] README status badge: "Paper submitted"

**Success check:** Paper compiles, video uploaded, repo tagged

---

## What Gets Cut if Time Short

**Priority 1 (Must have):**
- Sensor reading (Day 1)
- CWRU validation (Day 2)
- Dynamic clustering demo (Day 4)

**Priority 2 (Nice to have):**
- Real motor test (Day 3) → use CWRU only
- HITL corrections (Day 5) → document algorithm, defer experiment
- Cross-platform (Day 6) → show code compiles for both

**Priority 3 (Future work):**
- RapidSCADA integration → use mosquitto_sub
- Multiple fault types → just normal + outer race
- Power profiling → cut completely

## Current State

**Working:**
- Streaming k-means algorithm (200 lines C)
- Platform abstraction (ESP32 + RP2350)
- CWRU dataset pipeline (1904 samples converted)
- Unit tests passing in CI

**Not connected:**
- Sensor → algorithm (need to add ADXL345 to core.ino)
- Algorithm → MQTT (need to integrate MQTTConnector)
- MQTT → SCADA (need RapidSCADA config)

**7 days = connect the pieces**

## Risk Mitigation

**Risk 1 - Hardware failure:**
Fallback: CWRU dataset only, no motor test

**Risk 2 - MQTT issues:**
Fallback: Serial logging, post-process offline

**Risk 3 - Time overrun:**
Cut Priority 2/3, ship Priority 1 only

**Risk 4 - Accuracy too low:**
Document as "baseline for future HITL", show improvement potential

## Definition of Done

Paper shows:
1. ✓ Algorithm works (unit tests + CWRU)
2. ✓ Dynamic clustering (K grows 1→2→N)
3. ✓ Cross-platform (code compiles for both)
4. ✓ Open integration (MQTT demonstrated)
5. [OPTIONAL] Real hardware validation

Minimum viable submission: Items 1-4. Item 5 if time permits.