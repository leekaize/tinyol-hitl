# Experimental Protocol

## CWRU Dataset Validation

### Phase 1: Unsupervised Baseline (No HITL)
**Goal:** Measure clustering accuracy without corrections.

**Procedure:**
1. Load CWRU training set (140 samples)
2. Initialize k=4 clusters (random centroids)
3. Stream samples, update centroids
4. Evaluate on test set (60 samples)
5. Compute Normalized Mutual Information (NMI)

**Expected:** 70-85% accuracy (clusters misaligned with fault labels)

### Phase 2: 10% HITL Corrections
**Goal:** Measure improvement from operator feedback.

**Procedure:**
1. Label 14 training samples (10% of 140)
2. Send corrections via MQTT
3. Update centroids based on corrections
4. Re-evaluate on test set
5. Compute classification accuracy

**Expected:** 85-95% accuracy (+15-20% vs Phase 1)

### Phase 3: 20% HITL Corrections
**Goal:** Approach supervised performance.

**Procedure:**
1. Label additional 14 samples (total 20%)
2. Apply corrections
3. Final evaluation
4. Plot accuracy trajectory

**Expected:** 95-100% accuracy (match supervised baselines)

### Metrics Tracked
- Per-cluster inertia (within-cluster variance)
- Confusion matrix (predicted vs true labels)
- Latency: inference time, update time
- Memory: heap usage, stack high-water mark

### Reproducibility
- CWRU train/test split: `data/datasets/cwru/splits.json`
- Random seed: 42 (for centroid initialization)
- Hyperparameters: K=4, α_base=0.2, learning_rate_decay=0.01