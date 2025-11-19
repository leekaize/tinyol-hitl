# MQTT JSON Schema

Flat JSON for SCADA compatibility. No nested objects.

## Topic Structure

```
sensor/{device_id}/data          # Summary (every 10s)
tinyol/{device_id}/label         # Operator labels (button press)
tinyol/{device_id}/discard       # Operator discards (button press)
```

---

## Data Message (Device → SCADA)

**Published:** Every 10 seconds (summary of 100 samples)

### Normal Operation
```json
{
  "device_id": "tinyol_device1",
  "cluster": 0,
  "label": "normal",
  "k": 1,
  "alarm_active": false,
  "frozen": false,
  "sample_count": 100,
  "rms_avg": 5.2,
  "rms_max": 6.1,
  "peak_avg": 9.1,
  "peak_max": 11.2,
  "crest_avg": 1.75,
  "crest_max": 2.1,
  "buffer_samples": 0,
  "timestamp": 123456
}
```

### Alarm State
```json
{
  "device_id": "tinyol_device1",
  "cluster": -1,
  "label": "unknown",
  "k": 2,
  "alarm_active": true,
  "frozen": true,
  "sample_count": 100,
  "rms_avg": 12.3,
  "rms_max": 15.7,
  "peak_avg": 18.5,
  "peak_max": 22.1,
  "crest_avg": 2.45,
  "crest_max": 3.2,
  "buffer_samples": 100,
  "timestamp": 123456
}
```

**Field descriptions:**

| Field | Type | Description | Normal | Alarm |
|-------|------|-------------|--------|-------|
| `device_id` | string | Unique identifier | "tinyol_device1" | "tinyol_device1" |
| `cluster` | int | Assigned cluster (-1 if unknown) | 0 | -1 |
| `label` | string | Cluster label | "normal" | "unknown" |
| `k` | int | Total clusters | 1 | 2 |
| `alarm_active` | bool | Alarm triggered | false | true |
| `frozen` | bool | Buffer frozen | false | true |
| `sample_count` | int | Samples in window | 100 | 100 |
| `rms_avg` | float | Average RMS (m/s²) | 5.2 | 12.3 |
| `rms_max` | float | Maximum RMS (m/s²) | 6.1 | 15.7 |
| `peak_avg` | float | Average peak (m/s²) | 9.1 | 18.5 |
| `peak_max` | float | Maximum peak (m/s²) | 11.2 | 22.1 |
| `crest_avg` | float | Average crest factor | 1.75 | 2.45 |
| `crest_max` | float | Maximum crest factor | 2.1 | 3.2 |
| `buffer_samples` | int | Frozen buffer size | 0 | 100 |
| `timestamp` | int | Milliseconds since boot | 123456 | 123456 |

---

## Label Command (Operator → Device)

**Topic:** `tinyol/{device_id}/label`

**When:** Operator inspects motor, confirms fault, types label

```json
{
  "label": "outer_race_fault"
}
```

**Effect:**
- Device creates new cluster from frozen buffer
- K increases by 1
- Buffer cleared, sampling resumes
- Future similar patterns assigned to new cluster

**Validation:**
- Must be in FROZEN state
- Label cannot be empty
- Label cannot duplicate existing cluster
- K < MAX_CLUSTERS (16)

---

## Discard Command (Operator → Device)

**Topic:** `tinyol/{device_id}/discard`

**When:** Operator determines false alarm

```json
{
  "discard": true
}
```

**Effect:**
- Buffer cleared without creating cluster
- Sampling resumes
- K unchanged

---

## FUXA Configuration

### 1. Add MQTT Device

FUXA → **Connections** → **Add** → **MQTTclient**

- Name: `mqtt_broker`
- Broker: `mqtt://localhost:1883`
- Client ID: `fuxa_client_01`
- QoS: `0`

### 2. Subscribe to Topics

Device `mqtt_broker` → **Add** → **Search** → **Select Tags**

- Topic: `sensor/+/data`
- Type: `JSON`

### 3. Map JSON to Tags

All fields at root level (flat JSON):

| Tag Name | JSON Path | Type | Alarm Color |
|----------|-----------|------|-------------|
| device_id | `device_id` | string | - |
| cluster | `cluster` | int | Red if -1 |
| label | `label` | string | - |
| k | `k` | int | - |
| alarm_active | `alarm_active` | bool | Red if true |
| frozen | `frozen` | bool | - |
| rms_avg | `rms_avg` | float | Yellow >10 |
| rms_max | `rms_max` | float | Red >15 |
| peak_avg | `peak_avg` | float | Yellow >15 |
| peak_max | `peak_max` | float | Red >20 |
| crest_avg | `crest_avg` | float | Yellow >2 |
| crest_max | `crest_max` | float | Red >3 |
| buffer_samples | `buffer_samples` | int | - |

### 4. Create Dashboard

**Alarm Banner (conditional visibility):**
- Visible when: `alarm_active == true`
- Background: Red, flashing
- Text: "ANOMALY DETECTED - INSPECT MOTOR"
- Size: Full width, 80px height

**Status Display:**
- Cluster: Text widget → `label`
- K: Text widget → `k`
- Frozen: LED indicator → `frozen`

**Gauges (3x):**
- RMS: 0-20 range, bind to `rms_avg`
- Peak: 0-30 range, bind to `peak_avg`
- Crest: 0-5 range, bind to `crest_avg`

**Summary Table:**
- Columns: `timestamp`, `cluster`, `label`, `rms_avg`, `peak_avg`
- Last 20 rows (200 seconds history)
- Auto-scroll enabled

**Frozen Buffer Table (conditional visibility):**
- Visible when: `buffer_samples > 0`
- Shows: Detailed view of frozen samples
- Note: Requires separate MQTT topic for buffer data (future enhancement)

**Operator Controls:**
- Input field → Variable: `label_input`
- Button "Label" → MQTT Publish:
  - Topic: `tinyol/{device_id}/label`
  - Payload: `{"label":"${label_input}"}`
  - Enabled when: `alarm_active == true`
- Button "Discard" → MQTT Publish:
  - Topic: `tinyol/{device_id}/discard`
  - Payload: `{"discard":true}`
  - Enabled when: `alarm_active == true`

---

## Operator Workflow

### Step 1: Normal Monitoring
- Watch summary table
- RMS stays ~5 m/s², crest ~1.75
- No alarms

### Step 2: Anomaly Detected
- Red banner appears: "ANOMALY DETECTED"
- RMS jumps to 12.3 m/s²
- `frozen: true`, `buffer_samples: 100`
- Device stops sampling (waits for operator)

### Step 3: Physical Inspection
- Operator walks to motor (unlimited time)
- Checks:
  - Bearings (temperature, noise)
  - Vibration (hand-feel)
  - Visual inspection (oil leaks, cracks)
  - Smell (burning, overheating)

### Step 4: Decision
**If real fault:**
- Return to FUXA
- Type label: "bearing_outer_race"
- Click "Label"
- Device creates cluster, K=2
- Future samples auto-classify

**If false alarm:**
- Click "Discard"
- Device clears buffer
- Sampling resumes
- No cluster created

### Step 5: Confirmation
- Banner disappears
- `alarm_active: false`, `frozen: false`
- Next summary shows new cluster (if labeled)

---

## Testing

### Publish Normal Data
```bash
mosquitto_pub -h localhost -t "sensor/test_device/data" \
  -m '{
    "device_id":"test_device",
    "cluster":0,
    "label":"normal",
    "k":1,
    "alarm_active":false,
    "frozen":false,
    "sample_count":100,
    "rms_avg":5.2,
    "rms_max":6.1,
    "peak_avg":9.1,
    "peak_max":11.2,
    "crest_avg":1.75,
    "crest_max":2.1,
    "buffer_samples":0,
    "timestamp":123456
  }'
```

### Publish Alarm
```bash
mosquitto_pub -h localhost -t "sensor/test_device/data" \
  -m '{
    "device_id":"test_device",
    "cluster":-1,
    "label":"unknown",
    "k":1,
    "alarm_active":true,
    "frozen":true,
    "sample_count":100,
    "rms_avg":12.3,
    "rms_max":15.7,
    "peak_avg":18.5,
    "peak_max":22.1,
    "crest_avg":2.45,
    "crest_max":3.2,
    "buffer_samples":100,
    "timestamp":123456
  }'
```

**FUXA should:**
- Show red banner
- Enable "Label" and "Discard" buttons
- Display frozen values in gauges

### Send Label
```bash
mosquitto_pub -h localhost \
  -t "tinyol/test_device/label" \
  -m '{"label":"bearing_fault"}'
```

### Send Discard
```bash
mosquitto_pub -h localhost \
  -t "tinyol/test_device/discard" \
  -m '{"discard":true}'
```

### Subscribe to All
```bash
mosquitto_sub -h localhost -t "#" -v
```

---

## Troubleshooting

**Banner doesn't appear:**
- Check conditional visibility: `alarm_active == true`
- Verify tag binding: Click banner → Properties → Visibility

**Buttons don't publish:**
- Check topic string: Must match device_id exactly
- Verify payload JSON: No trailing commas
- Test with `mosquitto_sub` to confirm message sent

**Gauges show old values:**
- Tags update every 10s (summary interval)
- Check "Last Update" in tag list
- Verify QoS=0 for fastest delivery

**Device doesn't respond to label:**
- Serial Monitor should print: "✓ Labeled: bearing_fault (K=2)"
- Check subscription: `mosquitto_sub -t "tinyol/#"`
- Verify device is in FROZEN state

---

## Why Flat JSON?

**SCADA compatibility:**
- Many industrial systems don't parse nested objects
- Tag paths like `features[0]` fail in some PLCs
- Flat structure works everywhere

**Debugging:**
- Easier to log and inspect
- Copy-paste into Excel works
- No JSON path complexity

**Performance:**
- Faster parsing on microcontrollers
- Less memory overhead
- Simpler validation

**Example of nested (bad for SCADA):**
```json
{
  "features": {
    "rms": {"avg": 5.2, "max": 6.1},
    "peak": {"avg": 9.1, "max": 11.2}
  }
}
```

**Flat alternative (good for SCADA):**
```json
{
  "rms_avg": 5.2,
  "rms_max": 6.1,
  "peak_avg": 9.1,
  "peak_max": 11.2
}
```

All fields at root level. Direct mapping to SCADA tags.