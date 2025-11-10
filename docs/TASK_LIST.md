# TinyOL-HITL: 14-Day Sprint to Paper Submission

## Preliminary

- [ ] **[TASK] Finalise research components** ⚡ HIGH
- [ ] **[TASK] Algorithm designs with tests passed** ⚡ HIGH

## Week 1: Core Integration (Days 1-7)

### Day 1-2: Sensor Integration
- [ ] **[TASK] Integrate ADXL345 into core.ino** ⚡ HIGH
  - Wire ADXL345 to ESP32-S3
  - Add Adafruit_ADXL345_U library
  - Read 3-axis acceleration in loop()
  - Convert float → Q16.16 fixed-point
  - **Success:** Serial monitor shows live X/Y/Z values
  - **Estimate:** 8 hours

### Day 2-3: Feature Extraction
- [ ] **[TASK] Implement on-device feature computation** ⚡ HIGH
  - Create `feature_extractor.c/h`
  - Buffer 256 samples (circular buffer)
  - Compute RMS, kurtosis, crest factor, variance
  - Output 4D feature vector every 21ms
  - **Success:** Features match CWRU pipeline (±5%)
  - **Estimate:** 12 hours

### Day 3-4: Serial Streaming
- [ ] **[TASK] Implement dataset streaming via Serial** ⚡ HIGH
  - Python script: `tools/stream_dataset.py`
  - Read CWRU .bin files
  - Send header + samples via USB-CDC
  - Arduino: parse header, read samples
  - Handshake protocol (send ACK after each sample)
  - **Success:** Stream 140 samples without errors
  - **Estimate:** 10 hours

### Day 5: Baseline Validation
- [ ] **[TASK] Run unsupervised clustering on CWRU dataset** 🔵 MEDIUM
  - Stream training set (140 samples, K=4)
  - Log cluster assignments to Serial
  - Compute NMI (Normalized Mutual Information)
  - **Success:** NMI > 0.65 (70-85% accuracy expected)
  - **Estimate:** 6 hours

### Day 6-7: Documentation Updates
- [ ] **[TASK] Update docs to match paper.md** 🟢 LOW
  - Add EXPERIMENTAL_PROTOCOL.md
  - Add DECISIONS.md
  - Update README status section
  - Update ARCHITECTURE with HITL details
  - Update DATASETS with Serial approach
  - **Success:** All doc gaps from paper.md closed
  - **Estimate:** 4 hours

## Week 2: HITL + Validation (Days 8-14)

### Day 8-9: HITL Corrections
- [ ] **[FEATURE] Implement MQTT-based HITL corrections** ⚡ HIGH
  - Add PubSubClient library
  - Subscribe to `tinyol/{device_id}/correction`
  - Parse JSON correction messages
  - Implement reverse EMA centroid update
  - Persist to NVS/LittleFS
  - Publish ACK
  - **Success:** Manual correction changes cluster assignment
  - **Estimate:** 14 hours

### Day 10: Phase 2 Validation (10% HITL)
- [ ] **[TASK] Run 10% HITL correction experiment** 🔵 MEDIUM
  - Label 14 samples manually
  - Send corrections via MQTT
  - Re-evaluate test set
  - Compute confusion matrix
  - **Success:** Accuracy improves by +10-15%
  - **Estimate:** 6 hours

### Day 11: Phase 3 Validation (20% HITL)
- [ ] **[TASK] Run 20% HITL correction experiment** 🔵 MEDIUM
  - Label additional 14 samples
  - Apply corrections
  - Final evaluation
  - Plot accuracy trajectory
  - **Success:** Accuracy reaches 95-100%
  - **Estimate:** 6 hours

### Day 12: Performance Benchmarking
- [ ] **[TASK] Measure latency and memory footprint** 🔵 MEDIUM
  - Log inference time (distance computation)
  - Log update time (centroid update)
  - Measure heap usage (ESP.getFreeHeap())
  - Measure stack high-water mark
  - Compare ESP32 vs RP2350
  - **Success:** Latency < 2ms, Memory < 5KB
  - **Estimate:** 6 hours

### Day 13: Hardware Test Rig (Optional)
- [ ] **[TASK] Validate on physical motor** 🟡 NICE-TO-HAVE
  - Run motor at 1800 RPM (healthy baseline)
  - Collect 100 samples
  - Induce outer race fault (drill 0.2mm)
  - Collect 100 fault samples
  - Compare clusters to CWRU
  - **Success:** Fault forms distinct cluster
  - **Estimate:** 8 hours
  - **Fallback:** Skip if time constrained—CWRU is sufficient

### Day 14: Paper Finalization
- [ ] **[TASK] Complete experimental results in paper.md** ⚡ HIGH
  - Add Phase 1/2/3 accuracy numbers
  - Add confusion matrices
  - Add latency/memory tables
  - Add comparison to baselines
  - Generate PDF via Pandoc
  - **Success:** Paper ready for submission
  - **Estimate:** 6 hours

## Critical Path (Must Ship)
1. Sensor integration → Feature extraction → Serial streaming (Days 1-4)
2. CWRU baseline validation (Day 5)
3. HITL corrections (Days 8-9)
4. Phase 2/3 experiments (Days 10-11)
5. Paper finalization (Day 14)

## Nice-to-Have (Ship Later)
- Hardware test rig validation (Day 13)
- RP2350 platform comparison
- Power profiling
- SCADA integration examples

## Risk Mitigation
**Risk:** ADXL345 sensor malfunction
**Mitigation:** Use CWRU dataset only (already have .bin files)

**Risk:** HITL accuracy doesn't improve
**Mitigation:** Fall back to semi-supervised baseline (k-means + labeled centroids)

**Risk:** Serial streaming too slow
**Mitigation:** Pre-cache dataset in Flash (small subset)

## Daily Standup Questions
1. What did I ship yesterday?
2. What am I shipping today?
3. What's blocking me?

## Definition of Done
- [ ] All HIGH priority tasks complete
- [ ] CWRU experiments run (Phase 1, 2, 3)
- [ ] Paper has experimental results
- [ ] Code compiles on ESP32-S3 and RP2350
- [ ] README reflects current state

**Ship working code. Perfect later.**