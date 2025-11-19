# FUXA SCADA Integration

Open-source web-based SCADA with freeze-on-alarm workflow.

## Quick Start

### 1. Run FUXA + Mosquitto

```bash
# FUXA SCADA
docker run -d \
  -p 1881:1881 \
  -v fuxa_appdata:/usr/src/app/FUXA/server/_appdata \
  -v fuxa_db:/usr/src/app/FUXA/server/_db \
  --restart unless-stopped \
  frangoteam/fuxa:latest

# Mosquitto MQTT Broker
docker run -d \
  --name mosquitto \
  -p 1883:1883 \
  -v $(pwd)/mosquitto/config:/mosquitto/config \
  --restart unless-stopped \
  eclipse-mosquitto:latest
```

**Mosquitto config** (`mosquitto/config/mosquitto.conf`):
```
listener 1883
allow_anonymous true
```

**Access:** http://device-ip:1881

### 2. Add MQTT Device

FUXA → **Connections** → **Add** → **MQTTclient**

Config:
- Name: `mqtt_broker`
- Broker: `mqtt://localhost:1883`
- Client ID: `fuxa_client_01`
- QoS: `0` (fastest)

Click connection → Should see green circle.

### 3. Subscribe to Topics

Device `mqtt_broker` → **Add** → **Search** → **Select Tags**

- Topic: `sensor/+/data`
- Type: `JSON`

Test:
```bash
mosquitto_pub -h localhost -t "sensor/test/data" \
  -m '{"cluster":0,"label":"test","rms_avg":5.2}'
```

Check FUXA Device logs → Should show incoming message.

### 4. Map JSON to Tags

**All fields flat (no nesting):**

| Tag Name | JSON Path | Type |
|----------|-----------|------|
| alarm_active | `alarm_active` | bool |
| frozen | `frozen` | bool |
| cluster | `cluster` | int |
| label | `label` | string |
| k | `k` | int |
| rms_avg | `rms_avg` | float |
| rms_max | `rms_max` | float |
| peak_avg | `peak_avg` | float |
| peak_max | `peak_max` | float |
| crest_avg | `crest_avg` | float |
| crest_max | `crest_max` | float |
| buffer_samples | `buffer_samples` | int |

### 5. Create Dashboard

FUXA → **View** → **Add View** → Name: `Motor Monitor`

#### Layout

```
┌─────────────────────────────────────────┐
│  [RED ALARM BANNER]                     │  ← Visible when alarm_active
│  ANOMALY DETECTED - INSPECT MOTOR       │
│  [Label: ____] [Button: Label] [Discard]│
├─────────────────────────────────────────┤
│  Status: Cluster [0] "normal" | K: 1    │
├─────────────────────────────────────────┤
│  RMS:   [Gauge 0-20]   Peak: [Gauge]    │
│  Crest: [Gauge 0-5]                     │
├─────────────────────────────────────────┤
│  History (last 20 summaries):           │
│  [Table: time | cluster | rms | peak]   │
└─────────────────────────────────────────┘
```

#### Widgets

**1. Alarm Banner (Rectangle + Text)**
- Background: Red (#FF0000)
- Visibility condition: `alarm_active == true`
- Animation: Flashing (optional)
- Text: "ANOMALY DETECTED - INSPECT MOTOR"
- Font: Bold, 24px
- Position: Top, full width

**2. Input + Buttons**
- Input field:
  - Variable: `label_input`
  - Placeholder: "fault_name"
  - Width: 200px
- Button "Label":
  - Enabled when: `alarm_active == true`
  - Action: MQTT Publish
    - Topic: `tinyol/tinyol_device1/label`
    - Payload: `{"label":"${label_input}"}`
- Button "Discard":
  - Enabled when: `alarm_active == true`
  - Action: MQTT Publish
    - Topic: `tinyol/tinyol_device1/discard`
    - Payload: `{"discard":true}`

**3. Status Text**
- Cluster: Bind to `label` tag
- K: Bind to `k` tag
- Font: 18px

**4. Gauges (3x)**
- RMS:
  - Min: 0, Max: 20
  - Bind: `rms_avg`
  - Yellow: >10, Red: >15
- Peak:
  - Min: 0, Max: 30
  - Bind: `peak_avg`
  - Yellow: >15, Red: >20
- Crest:
  - Min: 0, Max: 5
  - Bind: `crest_avg`
  - Yellow: >2, Red: >3

**5. Table**
- Columns: `timestamp`, `cluster`, `label`, `rms_avg`, `peak_avg`, `crest_avg`
- Rows: 20 (last 200 seconds)
- Auto-scroll: On
- Refresh: 10s (matches publish rate)

Save view.

### 6. Test End-to-End

**Step 1: Start ESP32**
```bash
# Upload core.ino
# Serial Monitor shows: "Publishing summaries every 10s"
```

**Step 2: Monitor Normal Operation**
- FUXA table updates every 10s
- RMS ~5 m/s², crest ~1.75
- No alarm banner

**Step 3: Trigger Alarm**
- Shake sensor vigorously
- Or manually publish alarm:
```bash
mosquitto_pub -h localhost -t "sensor/tinyol_device1/data" \
  -m '{
    "alarm_active":true,
    "frozen":true,
    "cluster":-1,
    "label":"unknown",
    "k":1,
    "rms_avg":12.3,
    "rms_max":15.7,
    "buffer_samples":100
  }'
```

**Expected:**
- Red banner appears
- Buttons enabled
- ESP32 Serial: "⚠️ OUTLIER DETECTED - ALARM FROZEN"

**Step 4: Inspect (Simulated)**
- Walk away from desk (unlimited time)
- Come back after 5 minutes (system still frozen)

**Step 5: Label**
- Type in input: `bearing_fault`
- Click "Label"
- ESP32 Serial: `✓ Labeled: bearing_fault (K=2)`
- Banner disappears
- Next summary shows cluster 1

**Success:** System creates cluster, resumes sampling.

---

## Operator Workflow

### Normal Operation (10s summaries)

**Device publishes:**
```json
{
  "alarm_active": false,
  "frozen": false,
  "cluster": 0,
  "label": "normal",
  "rms_avg": 5.2,
  "buffer_samples": 0
}
```

**FUXA shows:**
- Green status
- Gauges stable
- Table updating every 10s

**Operator action:** None. Just monitor.

### Anomaly Detected

**Device detects outlier:**
- Distance > 2× cluster radius
- Freezes ring buffer (100 samples)
- Publishes alarm

**Device publishes:**
```json
{
  "alarm_active": true,
  "frozen": true,
  "cluster": -1,
  "label": "unknown",
  "rms_avg": 12.3,
  "buffer_samples": 100
}
```

**FUXA shows:**
- Red banner: "ANOMALY DETECTED"
- Buttons enabled
- Gauges show frozen values

**Device stops sampling.** Waits indefinitely for operator.

### Physical Inspection

**Operator walks to motor:**
- Check temperature (hand-feel)
- Listen for noise (grinding, squealing)
- Visual inspection (cracks, oil leaks)
- Check vibration (unusual patterns)

**Takes as long as needed:** Hours, shift change, whatever.

**Device remains frozen.** No timeout.

### Decision: Label or Discard

**Scenario A: Real Fault**
Operator confirms bearing issue.

1. Returns to FUXA
2. Types label: `bearing_outer_race`
3. Clicks "Label"

Device:
- Creates new cluster from frozen buffer
- K increases: 1 → 2
- Clears buffer
- Resumes sampling

Next summary shows cluster 1 (bearing_outer_race).

**Scenario B: False Alarm**
Operator finds nothing wrong. Just a transient vibration spike.

1. Clicks "Discard"

Device:
- Clears buffer without creating cluster
- K unchanged
- Resumes sampling

Next summary shows cluster 0 (normal).

---

## Architecture

### State Machine

```
NORMAL → (outlier) → FROZEN → (label) → NORMAL
                      ↓
                  (discard) → NORMAL
```

**NORMAL:**
- Sampling at 10 Hz
- Publishing summaries every 10s
- Checking for outliers

**FROZEN:**
- No sampling
- Buffer held (100 samples)
- Waiting for MQTT command

### Data Flow

```
ESP32/RP2350        Mosquitto       FUXA SCADA
    │                   │               │
    ├─ publish ────────>│               │
    │  every 10s        │               │
    │                   ├─ subscribe ──>│
    │                   │               │
    │ (outlier)         │               │
    ├─ alarm ──────────>│               │
    │  (frozen)         │               │
    │                   ├──────────────>│
    │                   │               │ [Red banner]
    │                   │               │ [Operator inspects]
    │                   │               │
    │                   │<─ label ──────┤
    │<─ subscribe ──────┤               │
    │                   │               │
    │ (resume)          │               │
    ├─ publish ────────>│               │
    │  cluster 1        │               │
```

### MQTT Topics

| Topic | Direction | Rate | Purpose |
|-------|-----------|------|---------|
| `sensor/{id}/data` | Device → FUXA | 10s | Summary statistics |
| `tinyol/{id}/label` | FUXA → Device | On button | Create cluster |
| `tinyol/{id}/discard` | FUXA → Device | On button | Clear buffer |

---

## Troubleshooting

### Banner doesn't appear on alarm

**Check:**
```bash
# 1. Verify tag updates
# FUXA → Connections → mqtt_broker → Tags
# alarm_active should be true

# 2. Check banner visibility condition
# Banner → Properties → Visibility
# Should be: alarm_active == true

# 3. Test manually
mosquitto_pub -h localhost -t "sensor/test/data" \
  -m '{"alarm_active":true}'
```

### Buttons don't work

**Check:**
```bash
# 1. Subscribe to label topic
mosquitto_sub -h localhost -t "tinyol/#" -v

# 2. Click button in FUXA
# Should see: tinyol/device1/label {"label":"test"}

# 3. Check button action config
# Button → Properties → Actions → MQTT Publish
# Topic must match exactly
```

### Device doesn't receive label

**Check ESP32 Serial Monitor:**
```
✓ MQTT connected
Subscribed: tinyol/tinyol_device1/label

# Click "Label" in FUXA
# Should see:
✓ Labeled: bearing_fault (K=2)
```

If not:
- Verify MQTT_BROKER in config.h
- Check WiFi connection
- Restart device

### Gauges show old values

**Normal.** Summaries publish every 10s, not real-time.

To verify updates:
```bash
mosquitto_sub -h localhost -t "sensor/#" -v

# Should see message every 10 seconds
```

### False alarms too frequent

**Adjust outlier threshold:**

```bash
# Default: 2× cluster radius
# Increase to 3× for less sensitivity
mosquitto_pub -h localhost \
  -t "tinyol/device1/threshold" \
  -m '{"multiplier":3.0}'
```

Or in code:
```cpp
kmeans_set_threshold(&model, 3.0f);
```

---

## Production Deployment

### Security

**Current setup:** Testing only. Anonymous access.

**Production config** (`mosquitto.conf`):
```
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
allow_anonymous false
password_file /mosquitto/config/passwords
```

**Add users:**
```bash
mosquitto_passwd -c passwords device1
mosquitto_passwd passwords device2
```

**FUXA config:**
- Port: 8883
- Protocol: `mqtts://`
- Username: `device1`
- Password: From file

### Backup

**Export FUXA config:**
```bash
# FUXA → Settings → Export → Download JSON
# Save as: fuxa_config_backup.json
```

**Restore:**
```bash
# FUXA → Settings → Import → Upload JSON
```

**Mosquitto backup:**
```bash
docker exec mosquitto cat /mosquitto/config/mosquitto.conf > backup.conf
docker cp backup.conf mosquitto:/mosquitto/config/mosquitto.conf
docker restart mosquitto
```

---

## References

- FUXA docs: https://frangoteam.github.io/
- Mosquitto docs: https://mosquitto.org/
- MQTT schema: `docs/mqtt_schema.md`
- Architecture: `docs/ARCHITECTURE.md`