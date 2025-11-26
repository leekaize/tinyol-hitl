# 2-Day Final Sprint

**Deadline:** Presentation ready
**Philosophy:** Ship what matters. Label what's missing. Don't fake results.

## Day 1: Baseline + Slides

### Morning (4 hours)

- [ ] **CWRU dataset streaming**
  ```bash
  cd data/datasets/cwru
  python3 download.py           # Get 4 files minimum
  python3 extract_features.py   # RMS, kurtosis, crest, variance
  python3 to_binary.py          # Q16.16 binary
  ```
  **Done when:** Binary files exist in `binary/`

- [ ] **Run baseline experiment**
  ```bash
  python3 tools/stream_dataset.py /dev/ttyUSB0 binary/normal.bin
  # Record: cluster assignments, accuracy for each fault type
  ```
  **Done when:** Numbers in spreadsheet for K=1, K=4, +corrections

### Afternoon (4 hours)

- [ ] **Update slides with actual numbers**
  - Replace `[TODO]` placeholders in `docs/presentation/slides.md`
  - Add confusion matrix screenshot

- [ ] **Record demo video** (2 minutes max)
  - Show FUXA dashboard
  - Trigger alarm (tap sensor)
  - Label outlier
  - Show K=2

**Done when:** `docs/presentation/slides.md` has no `[TODO]` for baseline results

---

## Day 2: Hardware + Polish

### Morning (4 hours)

- [ ] **Motor test** (if available)
  - 5 min baseline
  - Add eccentric weight
  - 5 min fault condition
  - Capture in Serial Monitor

- [ ] **Cross-platform check**
  - Upload to RP2350
  - Verify same binary gives same clusters

**Done when:** Screenshot of both MCUs showing matching output

### Afternoon (4 hours)

- [ ] **Finalize slides**
  - Review for flow
  - Practice 10-minute talk
  - Export PDF backup

- [ ] **Commit everything**
  ```bash
  git add .
  git commit -m "feat: final presentation ready"
  git tag v1.0-presentation
  git push --tags
  ```

**Done when:** Tag `v1.0-presentation` on GitHub

---

## What to Skip

| Task | Why Skip |
|------|----------|
| Current sensor integration | No hardware, 3D features sufficient |
| HITL correction experiments | Demo shows concept, exact % not critical |
| Production MQTT security | Research demo, not production |
| Paper final draft | Slides are deliverable, paper can follow |

---

## Placeholder Policy

For anything not done, use this format in slides:

```markdown
**TO-DO:** [What's needed]
**Why missing:** [Honest reason]
**Best estimate:** [Educated guess if applicable]
```

Example:
```
Accuracy with HITL corrections: [TO-DO: +15-25% expected based on literature]
```

---

## Emergency Fallback

If CWRU streaming fails:
1. Use pre-computed features from literature
2. Cite Rosa et al. 2024 benchmarks
3. Focus demo on state machine workflow (alarm → label → resume)

If motor unavailable:
1. Tap sensor to simulate vibration change
2. Label as "manual_perturbation"
3. Show K growing in FUXA

---

## Success Criteria

Presentation is ready when:

1. ✅ Slides render in sli.dev
2. ✅ Baseline accuracy number (even if estimated)
3. ✅ Demo video or live demo plan
4. ✅ All `[TODO]` items either filled or explicitly marked as future work
5. ✅ Can answer: "What's the main contribution?" in one sentence

**Main contribution:**
*"TinyOL-HITL enables unsupervised fault discovery on microcontrollers where operators label faults as they occur, eliminating pre-training requirements."*