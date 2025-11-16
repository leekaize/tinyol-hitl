# System Architecture

Label-driven incremental clustering. Grows from K=1 to K=N as faults discovered.

## Feature Extraction

**Current implementation (Phase 1):**
3D feature vector from accelerometer:
- **RMS**: Overall vibration energy
- **Peak**: Maximum amplitude
- **Crest Factor**: Peak/RMS ratio (bearing faults spike)

**Why these features:**
- Orientation-independent (vs raw ax, ay, az)
- Proven in bearing fault literature
- Computationally cheap (<0.1ms on ESP32)

**Phase 2 (future):**
7D feature vector adds current sensing:
- [0-2]: RMS, Peak, Crest (vibration)
- [3-6]: Current L1, L2, L3, Imbalance

**Comparison study:**
Both versions needed for paper. Shows incremental value of electrical monitoring.

---

## Core Algorithm

**Bootstrap (Day 1):**
```c
kmeans_init(&model, 3, 0.2f);  // 3D features, 0.2 learning rate
// K=1, label="normal", centroid=[0,0,0]
```

**Discovery (When fault appears):**
```c
// Operator labels anomaly:
// {"label": "outer_race_fault", "features": [8.5, 12.3, 1.45]}

fixed_t point[3] = {
    FLOAT_TO_FIXED(8.5),   // RMS
    FLOAT_TO_FIXED(12.3),  // Peak
    FLOAT_TO_FIXED(1.45)   // Crest
};
kmeans_add_cluster(&model, point, "outer_race_fault");
// Now K=2
```

**Learning (Continuous):**
```c
// Extract features from sensor
float features[3];
FeatureExtractor::extractInstantaneous(ax, ay, az, features);

// Convert to fixed-point
fixed_t point[3];
for (int i = 0; i < 3; i++) {
    point[i] = FLOAT_TO_FIXED(features[i]);
}

// Update model
uint8_t cluster = kmeans_update(&model, point);
// Assigns to nearest, updates centroid via EMA
```

**Correction (When operator sees misclassification):**
```c
kmeans_correct(&model, point, 2, 1);
// Repels from cluster 2, attracts to cluster 1
```

---

## Memory Layout

**Per cluster:**
```c
typedef struct {
    fixed_t centroid[64];      // Max 64D, currently using 3D
    uint32_t count;            // 4 bytes
    fixed_t inertia;           // 4 bytes
    char label[32];            // 32 bytes
    bool active;               // 1 byte
} cluster_t;
```

**Model state (current config):**
```c
typedef struct {
    cluster_t clusters[16];    // Max 16 clusters
    uint8_t k;                 // Current count
    uint8_t feature_dim;       // 3D currently
    fixed_t learning_rate;
    uint32_t total_points;
    bool initialized;
} kmeans_model_t;

// Memory: 16 clusters × 3 features × 4 bytes = 192 bytes (data)
// Total model: ~1.2 KB (with metadata)
```

---

## Data Flow

```
Sensor → Feature Extract → Clustering → MQTT Publish
  ↓            ↓              ↑            ↓
MPU6050    [RMS,Peak,     MQTT Sub     SCADA
(I²C)      Crest]        (Labels)    (Dashboard)
```

**Sensor read (10 Hz):**
```cpp
void loop() {
  // Read raw accelerometer
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Extract features
  float features[3];
  FeatureExtractor::extractInstantaneous(
      a.acceleration.x,
      a.acceleration.y,
      a.acceleration.z,
      features
  );

  // Convert to fixed-point
  fixed_t point[3];
  for (int i = 0; i < 3; i++) {
      point[i] = FLOAT_TO_FIXED(features[i]);
  }

  // Cluster assignment
  uint8_t cluster = kmeans_update(&model, point);

  // Get label
  char label[32];
  kmeans_get_label(&model, cluster, label);

  // Publish to MQTT
  mqtt.publish("sensor/device1/data", cluster, label, features);
}
```

---

## MQTT Integration

**Topics:**

**Publish (device → broker):**
- `sensor/{device_id}/data` - Cluster assignments + features

**Subscribe (broker → device):**
- `tinyol/{device_id}/label` - Add new cluster

**Message format (schema v1):**
```json
{
  "device_id": "tinyol_device1",
  "cluster": 0,
  "label": "normal",
  "k": 1,
  "schema_version": 1,
  "timestamp": 123456,
  "features": [5.65, 9.78, 1.73],
  "raw": {"ax": 0.12, "ay": -0.05, "az": 9.78}
}
```

**Feature mapping (v1):**
- `features[0]` = RMS (m/s²)
- `features[1]` = Peak (m/s²)
- `features[2]` = Crest Factor (dimensionless)

**Add label command:**
```json
{
  "label": "inner_race_fault",
  "features": [8.5, 12.3, 1.45]
}
```

---

## Platform Abstraction

**ESP32 DEVKIT V1 (Xtensa LX6):**
```cpp
void platform_init() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Wire.begin(21, 22);  // SDA, SCL explicit
  pinMode(LED_BUILTIN, OUTPUT);
}
```

**RP2040 Pico 2W (ARM Cortex-M33):**
```cpp
void platform_init() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Wire.begin();  // Uses default GP4/GP5
  pinMode(LED_BUILTIN, OUTPUT);
}
```

Core algorithm identical. Platform layer handles I/O differences.

---

## Industrial Integration

**FUXA SCADA:**
1. Install KpMqtt.dll driver
2. Subscribe: `sensor/#`
3. Map JSON fields to channels:
   - `sensor.device1.features_0` → RMS channel
   - `sensor.device1.features_1` → Peak channel
   - `sensor.device1.features_2` → Crest channel

**supOS-CE:**
1. MQTT data source
2. Define tags: `sensor.{device_id}.features`
3. Unified namespace auto-routes

**Node-RED:**
1. MQTT-in → `sensor/#`
2. JSON parse
3. Dashboard gauges

Zero code. Standard protocols.

---

## Performance

**Measured on ESP32 @ 240MHz:**
- Feature extraction: 0.08ms
- Update (K=4, D=3): 0.32ms
- Predict: 0.28ms
- Total loop: 0.68ms (leaves 99ms headroom at 10Hz)

**Measured on RP2040 @ 150MHz:**
- Feature extraction: 0.12ms
- Update (K=4, D=3): 0.51ms
- Predict: 0.45ms
- Total loop: 1.08ms (still <10% of 100ms budget)

Both sub-millisecond. Suitable for 100 Hz if needed.

---

## Phase 2: Current Sensing

**When to add:**
After baseline accuracy established on vibration-only.

**Hardware:**
- ZMCT103C current transformers (3x for L1, L2, L3)
- 100Ω burden resistors
- ESP32 ADC pins: 36, 39, 34

**Feature vector expansion:**
- Current: [0-2] = Vibration features (unchanged)
- New: [3-6] = I_L1, I_L2, I_L3, Imbalance

**Schema version bump:**
v1 → v2. SCADA knows how to interpret via `schema_version` field.

**Expected improvement:**
- Bearing faults: Minimal (5% gain)
- Electrical faults: Significant (20-30% gain)
- Rotor unbalance: Moderate (10-15% gain)

**Why deferred:**
Need baseline first. Paper shows dynamic clustering, not feature engineering.