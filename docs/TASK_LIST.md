# 2-Day Final Sprint

- **Deadline:** Presentation ready
- **Philosophy:** Ship working demo. Document honest results. Skip perfection.

## Critical Path (What Actually Matters)

```mermaid
flowchart LR
    A[Integrate Current Sensor] --> B[Run Motor Tests]
    B --> C[Record Demo Video]
    C --> D[Update Slides with Real Data]
    D --> E[Polish Paper]
```

## Day 1: Hardware Validation + Data Collection (12 hours)

### Hour 0-2: Current Sensor Integration
**Goal:** Merge ZMCT103C code into main sketch

- [x] Add current sensing to `core.ino`
- [x] Update `feature_extractor.h` for 7D features `[rms, peak, crest, i1, i2, i3, i_total]`
- [x] Test current readings with motor running
- [x] Verify readings change with load
- [ ] Update vibration features to more pragmatic set
- [ ] Proper freezing when motor stopped or button pressed

**Done when:** Serial Monitor shows current values updating when motor load changes

### Hour 2-4: Motor Test - Baseline Collection
**Goal:** Establish normal cluster baseline

- [ ] Run motor at 15Hz for 5 minutes
- [ ] Collect 3000 samples (10Hz × 300s)
- [ ] Save Serial output to CSV
- [ ] Verify cluster variance is stable

**Test protocol:**
```
1. Power on motor (VFD at 15Hz)
2. Wait 10s for warm-up
3. Start Serial logging
4. Record 5 minutes
5. Stop motor, save log
```

**Done when:** CSV file with 3000+ rows, single cluster C0

### Hour 4-6: Fault Injection - Unbalance Test
**Goal:** Trigger alarm, label fault, create K=2

- [ ] Install 100 g·mm eccentric weight
- [ ] Run motor at 50Hz
- [ ] Wait for alarm trigger
- [ ] Label as "unbalance" via MQTT or Serial
- [ ] Verify K=2 and correct classification

**Expected results:**
- RMS increase: 2-5× baseline
- Crest factor spike: >2.5
- Alarm within 10 seconds

**Done when:** Screenshot showing K=2, "unbalance" cluster active

### Hour 6-8: Multi-Condition Test
**Goal:** Demonstrate adaptability

- [ ] Speed variation test (25Hz, 40Hz, 60Hz)
- [ ] Capture how baseline adapts
- [ ] Log confusion matrix if possible

**Optional (if time):**
- [ ] Phase loss simulation (disconnect one phase briefly)
- [ ] Label as "phase_loss" (K=3)

**Done when:** Evidence of multiple conditions handled

### Hour 8-10: Record Demo Video
**Goal:** 2-minute video showing complete workflow

**Script:**
```
0:00 - Show hardware setup (motor, MCU, sensors)
0:20 - Show FUXA dashboard (normal state)
0:40 - Add eccentric weight to pulley
1:00 - Watch alarm trigger (red banner)
1:20 - Label fault: "unbalance"
1:40 - Show K=2 in dashboard
2:00 - Show correct prediction on next unbalance
```

**Done when:** `demo.mp4` under 2 minutes, shows complete HITL cycle

### Hour 10-12: Data Analysis + Documentation
**Goal:** Create actual benchmark numbers

- [ ] Calculate accuracy from test runs
- [ ] Create confusion matrix
- [ ] Document memory usage (use `sizeof()` in code)
- [ ] Measure loop latency (use `micros()`)

**Metrics to capture:**
```
| Metric              | Value     |
|---------------------|-----------|
| Model memory (RAM)  | XXX bytes |
| Loop latency        | XXX ms    |
| Baseline accuracy   | XX%       |
| +HITL accuracy      | XX%       |
| Samples to alarm    | XX        |
```

**Done when:** Numbers filled in, screenshots captured

---

## Day 2: Slides + Paper + Polish (12 hours)

### Hour 0-3: Update Slides with Real Data
**Goal:** Replace all [TODO] placeholders

- [ ] Update benchmark comparison table
- [ ] Add actual accuracy numbers
- [ ] Add memory footprint comparison
- [ ] Embed demo video or screenshot sequence

**Key slides to update:**
1. Model Performance (add real CWRU comparison)
2. Test Results (add confusion matrix)
3. Implementation (add memory breakdown)

### Hour 3-5: Literature Comparison Section
**Goal:** Position clearly against TinyOL

**Key points to emphasize:**

| Aspect | TinyOL (Ren 2021) | TinyOL-HITL (Ours) |
|--------|-------------------|---------------------|
| Pre-training | Required (offline) | None |
| Memory | ~100KB autoencoder | 2.5KB total |
| Classes | Fixed at deployment | Dynamic (K grows) |
| Platform | ARM Cortex-M4 only | ARM + Xtensa |
| HITL | None | Core feature |

- [ ] Add TinyOL citation with comparison
- [ ] Add CWRU benchmark citations
- [ ] Explain data leakage awareness (Rosa 2024)

### Hour 5-7: Paper Updates
**Goal:** Fill placeholders in paper.tex

- [ ] Update abstract with actual results
- [ ] Fill Table 1 (capability comparison)
- [ ] Update Section V (Experimental Validation)
- [ ] Add confusion matrix figure

**Sections needing numbers:**
- Section IV.A: Memory footprint
- Section V.A: CWRU accuracy
- Section V.B: Hardware test results
- Section VI: Discussion

### Hour 7-9: Cross-Platform Verification
**Goal:** Prove both MCUs work

- [ ] Flash same code to RP2350
- [ ] Run identical test
- [ ] Capture delta between ESP32 and RP2350 centroids
- [ ] Document any differences

**Done when:** Screenshot showing both MCUs with similar outputs

### Hour 9-10: Slide Practice + Polish
**Goal:** Smooth 10-minute presentation

- [ ] Time each section
- [ ] Identify cuts if over time
- [ ] Add speaker notes
- [ ] Export PDF backup

**Target timing:**
```
Intro + Problem:      2 min
State of Art:         1 min
Why Unsupervised:     1 min
Research Setup:       1 min
Test Rig + Model:     2 min
Results + Demo:       2 min
Conclusion:           1 min
------------------------
Total:               10 min
```

### Hour 10-12: Final Review + Commit
**Goal:** Everything committed and deployed

- [ ] Run `make test` (unit tests pass)
- [ ] Verify slides deploy to GitHub Pages
- [ ] Tag release: `git tag v1.0-presentation`
- [ ] Push everything
- [ ] Generate PDF exports as backup

**Done when:** Tag exists, slides live at GitHub Pages URL

---

## What to Skip (Time Constraints)

| Task | Why Skip |
|------|----------|
| OPC-UA integration | MQTT sufficient for demo |
| Full CWRU streaming | Motor test more compelling |
| Multiple fault types | Unbalance alone proves concept |
| Paper polish | Slides are deliverable |
| Battery/power tests | Out of scope |

---

## Emergency Fallbacks

### If motor test fails:
1. Use sensor tap/shake as "fault simulation"
2. Label as "manual_perturbation"
3. Still demonstrates HITL workflow

### If FUXA integration broken:
1. Use Serial Monitor + mosquitto_sub
2. Screenshot terminal output
3. Explain "production would use SCADA"

### If current sensors noisy:
1. Use vibration-only (3D features)
2. Note "current integration as future work"
3. Still valid research contribution

### If cross-platform fails:
1. Use ESP32 only
2. Note "RP2350 tested separately"
3. Commit ESP32 results

---

## Honest Reporting Guidelines

For any incomplete item, use this format in slides/paper:

```markdown
**Result:** [Actual measured value or "Not tested"]
**Expected:** [What literature suggests]
**Limitation:** [Why not achieved]
```

Example:
```markdown
**Accuracy:** 78% (baseline), 85% (+HITL)
**Expected:** 80-90% based on k-means literature
**Limitation:** Limited fault conditions tested
```

---

## Files to Modify Today

### Priority 1 (Must change):
- `core/core.ino` - Add current sensing
- `core/feature_extractor.h` - Add 7D features
- `slidev/slides.md` - Replace [TODO] with data
- `docs/BENCHMARKS.md` - Fill actual numbers

### Priority 2 (Should change):
- `docs/paper/paper.tex` - Update results section
- `README.md` - Update with final status

### Priority 3 (Nice to have):
- `integrations/README.md` - Verify FUXA steps work
- `docs/ARCHITECTURE.md` - Add current sensing diagram

---

## Success Criteria

Presentation ready when:

1. ✅ Demo video shows alarm → label → predict cycle
2. ✅ Slides have actual accuracy numbers (not [TODO])
3. ✅ TinyOL comparison table complete
4. ✅ Memory footprint documented
5. ✅ At least one cross-platform screenshot

**Core message:**
> "TinyOL-HITL starts unsupervised with K=1, operators label faults as discovered, system learns without pre-training. 2.5KB memory, runs on ESP32 and RP2350."