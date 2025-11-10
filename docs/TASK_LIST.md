# TinyOL-HITL: 7-Day Sprint to Paper

## Status: Algorithm + HITL tests passing in CI

## Critical Path (Ship First)

### Days 1-2: Data Pipeline
- [ ] Python script: stream CWRU .bin files via Serial (140 samples, 35/class)
- [ ] Arduino: parse header + samples, feed to kmeans_update(), ACK each sample
- [ ] End-to-end: 140 samples with zero corruption, log cluster assignments
- [ ] Feature extraction: circular buffer (256 samples) → RMS/kurtosis/crest/variance

### Days 3-4: CWRU Baseline
- [ ] Stream training set (K=4, unsupervised), compute NMI score (target: >0.65)
- [ ] Generate confusion matrix per fault type, log to CSV
- [ ] Validate: matches literature baseline accuracy (70-85%)

### Days 5-6: HITL Integration
- [ ] MQTT subscriber: parse correction JSON (cluster_id, label, confidence)
- [ ] Apply corrections: call kmeans_correct(), persist to NVS/LittleFS
- [ ] Phase 2: inject 10% corrections (14 samples), re-evaluate (target: +10-15% accuracy)
- [ ] Phase 3: inject 20% corrections (28 samples), measure saturation point

### Day 7: Final Validation
- [ ] Run complete pipeline: baseline → HITL corrections → final accuracy
- [ ] Generate paper figures: confusion matrices, accuracy vs correction rate
- [ ] Document results in docs/EXPERIMENTAL_PROTOCOL.md

## Optional (If Time Permits)
- [ ] ADXL345 sensor: read 3-axis accel, feed to feature extractor
- [ ] Hardware test rig: motor vibration → real-time clustering
- [ ] Power profiling: ESP32 vs RP2350 comparison

## Success Criteria
- Baseline NMI > 0.65 on CWRU test set
- HITL improves accuracy by +15-25%
- All tests pass in CI (core + HITL)
- Reproducible results (dataset + code in repo)