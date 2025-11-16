# MQTT JSON Schema

Reference for SCADA integration and operator commands.

## Topic Structure

```
sensor/{device_id}/data          # Device publishes here (10 Hz)
tinyol/{device_id}/label         # Operator sends labels here
```

---

## Data Message (Device → SCADA)

**Published:** Every 100ms (10 Hz)

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

**Field descriptions:**

| Field | Type | Description |
|-------|------|-------------|
| `device_id` | string | Unique device identifier |
| `cluster` | int | Assigned cluster ID (0 to k-1) |
| `label` | string | Human-readable label |
| `k` | int | Total number of clusters |
| `schema_version` | int | Feature schema version |
| `timestamp` | int | Milliseconds since boot |
| `features` | array | Extracted features (see schema below) |
| `raw` | object | Raw accelerometer (optional, for debugging) |

---

## Schema Version 1 (Current)

**Feature array dimensions:** 3D

**Mapping:**
- `features[0]` = RMS (m/s²) - Overall vibration energy
- `features[1]` = Peak (m/s²) - Maximum amplitude
- `features[2]` = Crest Factor (dimensionless) - Peak/RMS ratio

**Typical values:**
- **Healthy motor:** RMS ~2-5 m/s², Crest ~1.4-1.6
- **Mild unbalance:** RMS ~5-8 m/s², Crest ~1.5-1.8
- **Severe fault:** RMS >10 m/s², Crest >2.0

**SCADA channel mapping:**
```
sensor.device1.rms    ← features[0]
sensor.device1.peak   ← features[1]
sensor.device1.crest  ← features[2]
sensor.device1.cluster ← cluster
sensor.device1.label   ← label
```

---

## Schema Version 2 (Phase 2 - Future)

**Feature array dimensions:** 7D

**Mapping:**
- `features[0-2]` = Same as v1 (vibration)
- `features[3]` = Current L1 (A)
- `features[4]` = Current L2 (A)
- `features[5]` = Current L3 (A)
- `features[6]` = Current Imbalance (%)

**When to use:** After ZMCT103C sensors installed.

**Migration:** SCADA checks `schema_version` field, ignores extra features if not configured.

---

## Label Command (Operator → Device)

**Topic:** `tinyol/{device_id}/label`

**When to send:** Operator sees anomaly, wants to create new cluster.

```json
{
  "label": "outer_race_fault",
  "features": [8.5, 12.3, 1.45]
}
```

**Effect:**
- System creates new cluster (K increases by 1)
- Centroid initialized at provided feature vector
- Future similar patterns assigned to this cluster

**Validation:**
- Feature array length must match device schema version
- Label must be unique (no duplicates)
- K < MAX_CLUSTERS (16)

**Example workflow:**
1. Operator sees high vibration in SCADA
2. Copies current feature values: `[8.5, 12.3, 1.45]`
3. Publishes label command with descriptive name
4. Device creates cluster, subsequent samples cluster correctly

---

## Correction Command (Operator → Device)

**Topic:** `tinyol/{device_id}/label` (same as label command)

**When to send:** Device assigned sample to wrong cluster.

```json
{
  "point": [8.5, 12.3, 1.45],
  "old_cluster": 2,
  "new_cluster": 1
}
```

**Effect:**
- Old cluster centroid repels from point
- New cluster centroid attracts point
- Cluster counts updated

**Example workflow:**
1. Device assigns sample to cluster 2 (outer race)
2. Operator knows it's actually cluster 1 (inner race)
3. Publishes correction command
4. Model refines boundaries

---

## RapidSCADA Configuration

**Install MQTT driver:**
1. Copy `KpMqtt.dll` to `ScadaComm/Drivers/`
2. Restart SCADA Communicator

**Add device:**
1. Configuration → Devices → Add
2. Protocol: MQTT Client
3. Broker: `broker.hivemq.com:1883` (or your broker)
4. Subscribe: `sensor/#`

**Create channels:**
```
Channel 1: sensor.device1.cluster (int)
Channel 2: sensor.device1.label (string)
Channel 3: sensor.device1.rms (float)
Channel 4: sensor.device1.peak (float)
Channel 5: sensor.device1.crest (float)
```

**Create table view:**
- Timestamp
- Cluster ID
- Label
- RMS
- Peak
- Crest

**Refresh rate:** 100ms (matches device publish rate)

---

## supOS-CE Configuration

**Create MQTT data source:**
1. System → Data Source → Add
2. Type: MQTT
3. Broker: `broker.hivemq.com:1883`
4. Topics: `sensor/#`

**Define tags:**
```
sensor.device1.data (JSON object)
  ├── cluster (int)
  ├── label (string)
  └── features (array)
      ├── [0] → rms (float)
      ├── [1] → peak (float)
      └── [2] → crest (float)
```

**Create dashboard:**
- Time-series chart: RMS, Peak over 60s
- Gauge: Crest factor (alarm at >2.0)
- Text indicator: Current label

---

## Node-RED Flow

**Minimal flow:**
```
[mqtt-in] → [json] → [function] → [debug]
          → [dashboard chart]
```

**MQTT-in node:**
- Server: `broker.hivemq.com:1883`
- Topic: `sensor/+/data`
- Output: JSON

**Function node:**
```javascript
msg.payload = {
    rms: msg.payload.features[0],
    peak: msg.payload.features[1],
    crest: msg.payload.features[2],
    label: msg.payload.label
};
return msg;
```

**Chart node:**
- Type: Line
- X-axis: Last 60 seconds
- Y-axis: RMS, Peak

---

## Testing

**Publish test data (Python):**
```python
import paho.mqtt.client as mqtt
import json
import time

client = mqtt.Client()
client.connect("broker.hivemq.com", 1883)

msg = {
    "device_id": "test_device",
    "cluster": 0,
    "label": "normal",
    "k": 1,
    "schema_version": 1,
    "timestamp": int(time.time() * 1000),
    "features": [5.65, 9.78, 1.73]
}

client.publish("sensor/test_device/data", json.dumps(msg))
```

**Subscribe to all data (terminal):**
```bash
mosquitto_sub -h broker.hivemq.com -t "sensor/#" -v
```

**Add label (terminal):**
```bash
mosquitto_pub -h broker.hivemq.com \
  -t "tinyol/device1/label" \
  -m '{"label":"outer_race_fault","features":[8.5,12.3,1.45]}'
```

---

## Security Notes

**Public broker (testing only):**
- No authentication
- Data visible to anyone
- Use only for development

**Production deployment:**
- Private MQTT broker (Mosquitto, HiveMQ, etc.)
- TLS encryption (port 8883)
- Username/password authentication
- Access Control Lists (ACL) per device

**Example secure config:**
```cpp
#define MQTT_BROKER "mqtt.yourcompany.com"
#define MQTT_PORT 8883
#define MQTT_USER "device1"
#define MQTT_PASS "securepassword"
```

---

## Troubleshooting

**Device publishes but SCADA sees nothing:**
- Check topic spelling: `sensor/device1/data` (exact match)
- Verify broker address
- Check firewall (port 1883)

**SCADA shows old data:**
- Restart SCADA Communicator service
- Verify device is connected (LED blinks every 100ms)

**Labels not working:**
- Check subscribe topic: `tinyol/device1/label`
- Verify JSON syntax (use validator)
- Feature count must match schema version

**Performance issues:**
- Reduce publish rate (10 Hz → 5 Hz)
- Use QoS 0 (not 1 or 2)
- Increase broker memory limit