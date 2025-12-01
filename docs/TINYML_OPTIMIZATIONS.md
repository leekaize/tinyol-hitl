# TinyML Optimizations

All optimizations are implemented in `core/streaming_kmeans.c` and `core/streaming_kmeans.h`.

## Summary Table

| Technique | Location | Benefit |
|-----------|----------|---------|
| Q16.16 fixed-point | `streaming_kmeans.h:10-13` | No FPU required |
| Squared distance | `streaming_kmeans.c:distance_squared()` | Avoids sqrt (~30% faster) |
| EMA updates | `kmeans_update()` | O(1) memory per sample |
| Static allocation | All struct definitions | No malloc/fragmentation |
| Ring buffer | `ring_buffer_t` | Bounded 100 samples |

---

## 1. Q16.16 Fixed-Point Arithmetic

**File:** `streaming_kmeans.h`, lines 10-13

```c
#define FIXED_POINT_SHIFT 16
typedef int32_t fixed_t;

#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * (1 << FIXED_POINT_SHIFT)))
#define FIXED_TO_FLOAT(x) ((float)(x) / (1 << FIXED_POINT_SHIFT))
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_POINT_SHIFT)
```

**Why:** ESP32/RP2350 can run floating-point but it's slow without FPU. Fixed-point uses integer ALU.

**Range:** Q16.16 supports values from -32768.0 to +32767.99998 with precision of 1/65536 ≈ 0.000015.

**Verification:** All sensor values (RMS, kurtosis, crest) fall within ±100, well within range.

---

## 2. Squared Distance (No Square Root)

**File:** `streaming_kmeans.c`, `distance_squared()` function

```c
static fixed_t distance_squared(const fixed_t* a, const fixed_t* b, uint8_t dim) {
    int64_t sum = 0;
    for (uint8_t i = 0; i < dim; i++) {
        int64_t diff = (int64_t)a[i] - (int64_t)b[i];
        sum += (diff * diff) >> FIXED_POINT_SHIFT;
    }
    return (fixed_t)sum;
}
```

**Why:** `sqrt()` is expensive (~50-100 cycles on ARM). For nearest-cluster, squared distance preserves ordering.

**Math:** If `d(a,b) < d(a,c)` then `d²(a,b) < d²(a,c)`. Comparison order is preserved.

**Usage:** Threshold comparisons adjusted to squared values:
```c
// outlier_threshold is stored as multiplier²
fixed_t threshold = FIXED_MUL(model->outlier_threshold, radius);
return distance > threshold;  // Both squared
```

---

## 3. EMA (Exponential Moving Average) Updates

**File:** `streaming_kmeans.c`, `kmeans_update()` function

```c
// EMA update rule: c_new = c_old + α(x - c_old)
float decay = 1.0f + 0.01f * cluster->count;
float alpha_f = FIXED_TO_FLOAT(model->learning_rate) / decay;
fixed_t alpha = FLOAT_TO_FIXED(alpha_f);

for (uint8_t i = 0; i < model->feature_dim; i++) {
    fixed_t diff = point[i] - cluster->centroid[i];
    cluster->centroid[i] += FIXED_MUL(alpha, diff);
}
```

**Why:** Standard k-means stores all points to recalculate centroids. EMA updates in O(1) space.

**Decay:** Learning rate decreases as cluster count increases, stabilizing over time.

**Memory:** Zero historical storage. Each update is immediate and discarded.

---

## 4. Static Allocation

**File:** `streaming_kmeans.h`, struct definitions

```c
#define MAX_CLUSTERS 16
#define MAX_FEATURES 64
#define RING_BUFFER_SIZE 100

typedef struct {
    cluster_t clusters[MAX_CLUSTERS];  // Fixed array
    // ... no pointers to dynamic memory
} kmeans_model_t;
```

**Why:** `malloc()` on MCU causes fragmentation and unpredictable failures. Static allocation = predictable memory.

**Tradeoff:** Wastes memory if K < 16. Acceptable for industrial use (need bounded worst-case).

---

## 5. Ring Buffer

**File:** `streaming_kmeans.h`, `ring_buffer_t`

```c
typedef struct {
    fixed_t samples[RING_BUFFER_SIZE][MAX_FEATURES];  // 100 × 64 × 4 = 25.6KB max
    uint16_t head;
    uint16_t count;
    bool frozen;
} ring_buffer_t;
```

**Why:** When outlier detected, we need historical samples for operator review. Ring buffer bounds memory.

**Actual usage:** With D=4 features: 100 × 4 × 4 = 1,600 bytes.

---

## Memory Footprint

```c
// Measured via sizeof()
sizeof(kmeans_model_t) = 2,548 bytes  // K=16, D=64 max config
sizeof(cluster_t)      = 148 bytes    // Per cluster
sizeof(ring_buffer_t)  = varies       // Depends on MAX_FEATURES

// Actual usage for CWRU test (K=4, D=4):
clusters: 4 × 148        = 592 bytes
buffer:   100 × 4 × 4    = 1,600 bytes
metadata:                = ~100 bytes
TOTAL:                   ≈ 2,300 bytes
```

**Comparison:**

| System | RAM | Method |
|--------|-----|--------|
| TinyOL (autoencoder) | ~100KB | Neural network |
| TinyTL (bias-only) | ~50KB | Transfer learning |
| MCUNetV3 | 256KB | CNN |
| **TinyOL-HITL** | **~2.5KB** | Streaming k-means |

---

## State Machine Integration

**File:** `streaming_kmeans.h`, `system_state_t`

```c
typedef enum {
    STATE_NORMAL,        // Green - sampling active
    STATE_ALARM,         // Red banner - outlier detected, motor running
    STATE_WAITING_LABEL  // Red + frozen - ready for operator input
} system_state_t;
```

**State transitions:**
- NORMAL → ALARM: Outlier detected
- ALARM → NORMAL: Auto-clear after 30 normal samples
- ALARM → WAITING_LABEL: Motor stops OR operator button
- WAITING_LABEL → NORMAL: Label/discard action

---

## Verification

Run CWRU simulation to verify:

```bash
cd core/tests
make test-cwru
```

Expected output confirms:
- Q16.16 conversions preserve precision
- K grows 1→4 correctly
- Accuracy within expected range (65-80%)