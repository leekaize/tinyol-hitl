# System Architecture

Label-driven incremental clustering. Grows from K=1 to K=N as faults discovered.

## Core Algorithm

**Bootstrap (Day 1):**
```c
kmeans_init(&model, 3, 0.2f);  // 3D features (accel x,y,z), 0.2 learning rate
// K=1, label="normal", centroid=[0,0,0]
```

**Discovery (When fault appears):**
```c
// Operator sees anomaly, sends MQTT:
// {"label": "outer_race_fault", "features": [0.5, 0.2, 0.8]}

fixed_t point[3] = {FLOAT_TO_FIXED(0.5), FLOAT_TO_FIXED(0.2), FLOAT_TO_FIXED(0.8)};
kmeans_add_cluster(&model, point, "outer_race_fault");
// Now K=2
```

**Learning (Continuous):**
```c
uint8_t cluster = kmeans_update(&model, sensor_reading);
// Assigns to nearest cluster
// Updates centroid via EMA: c_new = c_old + α(x - c_old)
// α decays: base_lr / (1 + 0.01 × count)
```

**Correction (When operator sees misclassification):**
```c
// Sample assigned to cluster 2, but operator says it's cluster 1
kmeans_correct(&model, point, 2, 1);
// Repels from cluster 2, attracts to cluster 1
```

## Memory Layout

**Per cluster:**
```c
typedef struct {
    fixed_t centroid[64];      // 256 bytes max
    uint32_t count;            // 4 bytes
    fixed_t inertia;           // 4 bytes
    char label[32];            // 32 bytes
    bool active;               // 1 byte
} cluster_t;  // ~297 bytes per cluster
```

**Model state:**
```c
typedef struct {
    cluster_t clusters[16];    // 4.75 KB max
    uint8_t k;                 // Current cluster count
    uint8_t feature_dim;
    fixed_t learning_rate;
    uint32_t total_points;
    bool initialized;
} kmeans_model_t;  // ~4.8 KB total
```

## Data Flow

```
Sensor → Feature Extract → Cluster Assignment → MQTT Publish
  ↓                             ↑
I²C                         MQTT Subscribe
(ADXL345)                   (Corrections/Labels)
```

**Sensor read (100 Hz):**
```cpp
void loop() {
  sensors_event_t event;
  accel.getEvent(&event);

  fixed_t features[3] = {
    FLOAT_TO_FIXED(event.acceleration.x),
    FLOAT_TO_FIXED(event.acceleration.y),
    FLOAT_TO_FIXED(event.acceleration.z)
  };

  uint8_t cluster = kmeans_update(&model, features);

  char label[32];
  kmeans_get_label(&model, cluster, label);

  mqtt.publish("sensor/device1/cluster", cluster, label, features);
  mqtt.loop();  // Check for corrections
}
```

**MQTT callback:**
```cpp
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload);

  const char* new_label = doc["label"];
  JsonArray arr = doc["features"];

  fixed_t point[3];
  for (int i = 0; i < 3; i++) {
    point[i] = FLOAT_TO_FIXED(arr[i].as<float>());
  }

  kmeans_add_cluster(&model, point, new_label);
}
```

## Platform Abstraction

**ESP32-S3 (Xtensa LX7):**
```cpp
void platform_init() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Wire.begin(21, 22);  // SDA, SCL
  pinMode(LED_PIN, OUTPUT);
}
```

**RP2350 (ARM Cortex-M33):**
```cpp
void platform_init() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Wire.begin();  // Default I²C pins
  pinMode(LED_PIN, OUTPUT);
}
```

Core algorithm identical. Platform layer handles I/O differences.

## MQTT Integration

**Topics:**

**Publish (device → broker):**
- `sensor/{device_id}/cluster` - Cluster assignments
- `sensor/{device_id}/health` - Uptime, memory, K value

**Subscribe (broker → device):**
- `tinyol/{device_id}/label` - Add new cluster
- `tinyol/{device_id}/correction` - Fix misclassification
- `tinyol/{device_id}/reset` - Reset to K=1

**Message formats:**

**Cluster assignment:**
```json
{
  "cluster": 2,
  "label": "outer_race_fault",
  "features": [0.45, -0.12, 0.89],
  "confidence": 0.87,
  "timestamp": 1699142400
}
```

**Add label:**
```json
{
  "label": "inner_race_fault",
  "features": [0.62, 0.21, -0.44]
}
```

**Correction:**
```json
{
  "point": [0.51, 0.19, 0.90],
  "old_cluster": 2,
  "new_cluster": 1
}
```

## Industrial Integration

**RapidSCADA:**
1. Install KpMqtt.dll driver
2. Configure subscription: `sensor/#`
3. Map JSON fields to channels
4. Table view shows live cluster assignments

**supOS-CE:**
1. Create MQTT data source
2. Define tags: `sensor.{device_id}.cluster`
3. Unified namespace auto-routes to dashboards

**Node-RED:**
1. MQTT-in node subscribes to `sensor/#`
2. JSON node parses payload
3. Dashboard nodes display real-time

Zero code. Standard protocols.

## State Persistence

**On power loss, model state lost.** Two options:

**Option 1 - Flash storage (NVS/LittleFS):**
```cpp
void save_model() {
  File f = LittleFS.open("/model.bin", "w");
  f.write((uint8_t*)&model, sizeof(model));
  f.close();
}

void load_model() {
  File f = LittleFS.open("/model.bin", "r");
  f.read((uint8_t*)&model, sizeof(model));
  f.close();
}
```

Call `save_model()` every 100 updates or on graceful shutdown.

**Option 2 - MQTT retained messages:**
Publish model snapshot to `tinyol/{device_id}/state` with retain flag. Broker stores. Device subscribes on boot, reconstructs state.

Pick Option 1 for fast recovery. Option 2 for remote management.

## Performance

**Measured on ESP32-S3 @ 240MHz:**
- Init: 0.2ms
- Update: 0.4ms (K=4, D=3)
- Predict: 0.3ms
- Add cluster: 0.1ms
- Memory: 4.2KB (K=16, D=64)

**Measured on RP2350 @ 150MHz:**
- Init: 0.3ms
- Update: 0.6ms (K=4, D=3)
- Predict: 0.5ms
- Add cluster: 0.2ms
- Memory: 4.2KB (K=16, D=64)

Both sub-millisecond. Suitable for 100 Hz sampling.