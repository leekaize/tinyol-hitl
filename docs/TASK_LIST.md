# 7-Day Sprint to Paper Submission

## Status: Algorithm updated with freeze-on-alarm workflow

## Day 1: FUXA SCADA Integration ✓

**Goal:** Live cluster monitoring with freeze-on-alarm

- [x] Add sensor to core.ino (MPU6050/ADXL345)
- [x] Implement freeze-on-alarm state machine
- [x] Flat JSON schema (SCADA compatible)
- [x] 10-second summary publishing (not 100ms streaming)
- [x] Ring buffer for alarm state (100 samples)
- [x] Install FUXA SCADA
- [x] Install Mosquitto broker
- [x] Configure MQTT device in FUXA
- [ ] Create dashboard with alarm banner and freeze buttons
- [ ] Test end-to-end: alarm → freeze → label → resume

**Next:** Complete FUXA dashboard with freeze workflow

**Status:** Algorithm ready, FUXA partially configured

---

## Day 2: CWRU baseline validation

**Goal:** Prove unsupervised clustering works

- [ ] Download CWRU dataset (~2GB)
- [ ] Extract features (RMS, kurtosis, crest, variance)
- [ ] Convert to binary format
- [ ] Stream via Serial to ESP32
- [ ] Log cluster assignments
- [ ] Compute confusion matrix (4 classes)
- [ ] Compare to literature baseline

**Success check:** Reliable unsupervised classification

**Commands:**
```bash
cd data/datasets/cwru
pip3 install -r requirements.txt
python3 download.py
python3 extract_features.py --input raw/ --output features/
python3 to_binary.py --input features/ --output binary/
python3 tools/stream_dataset.py /dev/ttyUSB0 binary/normal.bin
```

---

## Day 3: Real motor baseline

**Goal:** Hardware validation

- [ ] Wire ADXL345/MPU6050 to motor housing
- [ ] Run motor 5 min @ 1500 RPM (no weights = healthy)
- [ ] Verify tight clustering (>95% in cluster 0)
- [ ] Mount pulley for eccentric weight testing

**Success check:** Baseline cluster well-defined

**Fallback:** If motor unavailable, synthetic data or CWRU only

---

## Day 4: Label-driven growth demo

**Goal:** Prove dynamic clustering (K=1 → K=N)

**Scenario 1 - Mild Unbalance:**
1. Add 50 g·mm eccentric weight
2. Run 2 min, observe RMS increase in FUXA
3. System triggers alarm (outlier detected)
4. Operator inspects motor
5. Label: `mild_unbalance`
6. Verify K=2, new cluster formed

**Scenario 2 - Different Operating Condition:**
1. Change speed to 1800 RPM
2. Alarm triggers (different vibration signature)
3. Label: `high_speed_normal`
4. Verify K=3

**Success check:** System grows organically, operator-guided

---

## Day 5: HITL correction validation

**Goal:** Show operator feedback improves accuracy

- [ ] Re-run CWRU stream (baseline accuracy)
- [ ] Inject 5 intentional misclassifications via correction command
- [ ] Measure accuracy improvement
- [ ] Target: Measurable gain from corrections

**Correction format:**
```json
{"point": [0.5,0.2,0.8], "old_cluster": 2, "new_cluster": 1}
```

**Success check:** Accuracy improves with human guidance

---

## Day 6: Cross-platform validation

**Goal:** Prove portability (Xtensa + ARM)

- [ ] Upload to RP2350 (Pico 2 W)
- [ ] Stream same CWRU dataset
- [ ] Compare cluster assignments: ESP32 vs RP2350
- [ ] Measure delta: max(|c_ESP32 - c_RP2350|)

**Success check:** Delta <0.1 (fixed-point consistency)

**Fallback:** Document code compiles for both platforms

---

## Day 7: Paper finalization

**Goal:** Camera-ready submission

**Morning:**
- [ ] Update Results section with actual numbers
- [ ] Generate figures (confusion matrices, cluster plots)
- [ ] Replace placeholders with data
- [ ] Compile PDF, verify citations

**Afternoon:**
- [ ] Record demo video (3 min):
  - FUXA dashboard with live data
  - Alarm triggered
  - Operator labels outlier
  - Cluster grows (K=1 → K=2)
- [ ] Push to GitHub with tag `v1.0-paper`
- [ ] Update README with results

**Success check:** Paper ready, demo recorded

---

## Phase 2: Current Sensing (Post-Paper)

**Why deferred:** Need vibration baseline first

**When sensors arrive:**
- [ ] Wire ZMCT103C to L1, L2, L3
- [ ] Update feature vector: 3D → 7D
- [ ] Re-run experiments
- [ ] Measure improvement vs vibration-only

**Hypothesis:** Current adds value for electrical faults

---

## Definition of Done

**Must have (for paper):**
1. [x] Algorithm works (tests pass)
2. [x] Cross-platform Arduino library
3. [x] Freeze-on-alarm workflow implemented
4. [ ] FUXA dashboard operational
5. [ ] Dynamic clustering demo (K grows)
6. [ ] CWRU validation (baseline accuracy)

**Nice to have:**
- [ ] Real motor test (if time permits)
- [ ] HITL corrections (document algorithm)
- [ ] Cross-platform hardware test

**Minimum for submission:** Items 1-6. Rest is future work.

---

## Current Progress

**Completed:**
- ✓ Streaming k-means with freeze-on-alarm
- ✓ Platform abstraction (ESP32 + RP2350)
- ✓ Feature extraction (RMS, peak, crest)
- ✓ Flat JSON schema
- ✓ MQTT publish/subscribe
- ✓ Ring buffer for alarm state
- ✓ Outlier detection (distance threshold)
- ✓ Unit tests in CI
- ✓ Paper title/abstract submitted
- ✓ FUXA + Mosquitto installed

**In progress:**
- FUXA dashboard creation (alarm banner, buttons)

**Next 24 hours:**
- Complete FUXA dashboard
- Test freeze-on-alarm workflow end-to-end
- Start CWRU dataset download

**7 days remaining.** On track for paper submission.

---

## Architecture Summary

**State machine:**
```
NORMAL → (outlier) → FROZEN → (label) → NORMAL + K++
                      ↓
                  (discard) → NORMAL
```

**Key features:**
- No timeouts (operator has unlimited time to inspect)
- Flat JSON (SCADA compatibility)
- 10-second summaries (not real-time streaming)
- Ring buffer holds 100 samples on alarm
- Device learns automatically, operator labels outliers only

**Memory:** <2.5 KB (model + buffer)
**Latency:** <20 ms per loop
**MQTT traffic:** ~9 KB/hour

Pragmatic. Industrial-ready. Operator-friendly.