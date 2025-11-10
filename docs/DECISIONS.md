# Architectural Decisions

## [DECISION-001] Serial vs SD Card for Dataset Streaming

**Context:** Need to stream CWRU .bin files to MCU for validation.

**Options:**
- **A) SD Card:** Read from SD via SPI
- **B) Serial:** Stream from PC via USB
- **C) Flash:** Embed dataset in PROGMEM

**Decision:** Serial (Option B)

**Reasoning:**
- No hardware dependencies (SD card reader, wiring)
- Fast iteration (10s to restart vs reflashing)
- Works on all platforms (USB-CDC universal)
- Dataset is 2GB—won't fit in Flash anyway

**Tradeoff:** Requires PC tethered during tests. Acceptable for validation phase.

---

## [DECISION-002] Raw Accel vs On-Device Features

**Context:** ADXL345 gives 3-axis acceleration. Paper uses 4 features (RMS, kurtosis, crest, variance).

**Options:**
- **A) Raw:** Feed X/Y/Z directly to k-means
- **B) Features:** Compute RMS/kurtosis on-device

**Decision:** Features (Option B)

**Reasoning:**
- Matches CWRU methodology (apples-to-apples)
- Dimensionality reduction (4D vs 3D × 256 samples)
- Literature shows features outperform raw for bearing faults

**Implementation:** Buffer 256 samples, compute features every 21ms.

---

## [DECISION-003] HITL Correction Strategy

**Context:** Need to update centroids based on operator corrections.

**Options:**
- **A) Reverse EMA:** Undo incorrect assignments, reassign
- **B) Hard reset:** Replace centroid with corrected sample
- **C) Weighted merge:** Blend old/new based on confidence

**Decision:** Reverse EMA (Option A)

**Reasoning:**
- Preserves stability (doesn't throw away learned patterns)
- Handles batch corrections (multiple samples to same cluster)
- Mathematically sound (inverse of update rule)

**Formula:**

```
c_new = c_old - α(x_wrong - c_old) + α(x_correct - c_old)
= c_old + α(x_correct - x_wrong)
```