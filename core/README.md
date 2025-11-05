# Streaming K-Means Core

Memory-efficient incremental clustering for edge devices.

## Design

**Algorithm:** Exponential moving average centroid updates  
**Memory:** K × feature_dim × 4 bytes + metadata  
**Precision:** Q16.16 fixed-point (ARM optimized)

**Update rule:**
```
c_new = c_old + α(point - c_old)
α = base_lr / (1 + 0.01 × count)
```

## Files

- `streaming_kmeans.h` - API interface
- `streaming_kmeans.c` - Core implementation  
- `test_kmeans.c` - Unit tests

## Build Tests

```bash
gcc -o test_kmeans test_kmeans.c streaming_kmeans.c -lm
./test_kmeans
```

## Configuration

Edit `streaming_kmeans.h`:
```c
#define MAX_CLUSTERS 16   // Max K
#define MAX_FEATURES 64   // Max dimensions
```

## Memory Footprint

- 10 clusters × 32 features = 1.28 KB
- 16 clusters × 64 features = 4.2 KB
- Scales linearly: K × D × 4 bytes

## API Example

```c
kmeans_model_t model;
kmeans_init(&model, 3, 2, 0.2f);

fixed_t point[2] = {
    FLOAT_TO_FIXED(0.5f), 
    FLOAT_TO_FIXED(-0.3f)
};

uint8_t cluster = kmeans_update(&model, point);
```

## Validation

Test suite covers:
- Convergence (synthetic 2D/32D clusters)
- Memory constraints (<100KB target)
- Inertia tracking
- Edge cases (invalid params, reset)

Target: <5KB RAM, 150+ points/sec on RP2350 @ 150MHz.