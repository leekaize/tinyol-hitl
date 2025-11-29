# 2-Day Final Sprint (Updated)

## Critical Path

```mermaid
flowchart LR
    A[FFT Research] --> B[Update Features]
    B --> C[Motor Tests]
    C --> D[Record Demo]
    D --> E[Update Results]
    E --> F[Polish Paper/Slides]
```

---

## Completed

- [x] Current sensor integration (`core.ino`)
- [x] 7D feature extraction (`feature_extractor.h`)
- [x] State machine redesign (NORMAL → ALARM → WAITING_LABEL)
- [x] MQTT schema update (alarm_active, waiting_label, motor_running)
- [x] Architecture documentation
- [x] Slides update with new state machine
- [x] Feature schema comparison design (3D vs 7D vs 6D vs 10D)

---

## Day 1 Remaining

### Hour 0-2: FFT Feature Research

**Action:** Research minimal FFT features.

**Done when:**
- [ ] FFT feature list with citations
- [x] Update `feature_extractor.h` with actual implementation
- [x] Test FFT on ESP32

### Hour 2-4: Motor Baseline Test

**Protocol:**
1. Power on VFD at 50 Hz
2. Wait 30s warm-up
3. Start Serial logging
4. Record 5 min (3000 samples @ 10Hz)
5. Verify single cluster (K=1)

**Done when:**
- [ ] CSV with 3000+ samples
- [ ] Screenshot: K=1, stable variance

### Hour 4-6: Fault Injection Test

**Protocol:**
1. Install 100 g·mm eccentric weight
2. Resume motor at 50 Hz
3. Watch for ALARM state
4. Let motor run → observe ALARM (sampling continues)
5. Stop motor → observe WAITING_LABEL (frozen)
6. Send label via MQTT
7. Verify K=2

**Done when:**
- [ ] Screenshot: ALARM state
- [ ] Screenshot: WAITING_LABEL state
- [ ] Screenshot: K=2 after labeling

### Hour 6-8: Record Demo Video

**Script (2 min):**
```
0:00 - Hardware: motor, MCU, sensors
0:20 - FUXA: NORMAL state
0:40 - Add weight → ALARM (red banner, still sampling)
1:00 - Motor stops → WAITING_LABEL (frozen)
1:20 - Label: "unbalance"
1:40 - K=2, resume NORMAL
2:00 - Next unbalance → auto-classify to C1
```

**Done when:**
- [ ] `demo.mp4` under 2 min

### Hour 8-10: Schema Comparison (CWRU)

**Test each schema:**
1. TIME_ONLY (3D): rms, peak, crest
2. TIME_CURRENT (7D): + i1, i2, i3, i_rms
3. FFT_ONLY (6D): + fft_freq, fft_amp, centroid

**Done when:**
- [ ] Accuracy for each schema
- [ ] Confusion matrix for best schema

### Hour 10-12: Fill Results Placeholders

Files to update:
- [ ] `slidev/slides.md` - [TODO] → actual numbers
- [ ] `docs/BENCHMARKS.md` - fill tables
- [ ] Screenshot gallery for paper

---

## Day 2

### Hour 0-3: Update Paper

Sections needing data:
- Abstract: [PLACEHOLDER] → actual results
- Section V.A: CWRU accuracy per schema
- Section V.B: Hardware test results
- Section VI: Discussion

### Hour 3-5: Cross-Platform Test

- [ ] Flash RP2350 with same firmware
- [ ] Run identical 100-sample test
- [ ] Compare centroid delta vs ESP32
- [ ] Document any differences

### Hour 5-7: Final Slide Polish

- [ ] Time each section (target: 10 min total)
- [ ] Add speaker notes
- [ ] Export PDF backup

### Hour 7-10: Paper Finalization

- [ ] Run `make` in docs/paper
- [ ] Fix any LaTeX errors
- [ ] Add figures from screenshots

### Hour 10-12: Final Review

- [ ] `make test` passes
- [ ] Slides deploy to GitHub Pages
- [ ] Tag release: `v1.0-presentation`
- [ ] Push everything

---

## Research Comparison Summary

| Experiment | Schema | Features |
|------------|--------|----------|
| Exp 1 | TIME_ONLY | rms, peak, crest |
| Exp 2 | TIME_CURRENT | + 3-phase current |
| Exp 3 | FFT_ONLY | + FFT features |
| Exp 4 | FFT_CURRENT | All combined |

**Hypothesis:**
- FFT adds 5-10% accuracy (frequency signatures)
- Current adds 5-10% accuracy (load correlation)
- Combined adds 10-15% over baseline

---

## Files Updated This Session

| File | Status |
|------|--------|
| `core/streaming_kmeans.h` | ✓ New state machine |
| `core/streaming_kmeans.c` | ✓ New implementation |
| `core/feature_extractor.h` | ✓ Schema selection |
| `docs/ARCHITECTURE.md` | ✓ Updated diagrams |
| `docs/mqtt_schema.md` | ✓ New fields |
| `slidev/slides.md` | ✓ Updated |
| `docs/TASK_LIST.md` | ✓ This file |

## Files Still Need Update

| File | What's Needed |
|------|---------------|
| `core/core.ino` | Use new API |
| `core/tests/test_kmeans.c` | Update for new states |
| `docs/paper/paper.tex` | Add results |
| `docs/BENCHMARKS.md` | Fill numbers |

---

## Emergency Fallbacks

**If FFT too complex:**
- Use time-domain only (proven works)
- Note "FFT as future work"

**If current sensors noisy:**
- Compare vibration-only results
- Still valid contribution

**If motor test fails:**
- Use tap/shake as "fault"
- Demonstrates HITL workflow