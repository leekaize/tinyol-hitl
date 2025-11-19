# FUXA SCADA Integration

Open-source web SCADA. Freeze-on-alarm workflow. Label outliers via dashboard.

## Setup Flow

```mermaid
graph TD
    A[Start] --> B[Run FUXA + Mosquitto]
    B --> C[Add MQTT device]
    C --> D[Subscribe to topics]
    D --> E[Map JSON to tags]
    E --> F[Create dashboard]
    F --> G[Test workflow]

    style G fill:#90EE90
```

## Quick Start

```bash
# FUXA SCADA
docker run -d -p 1881:1881 \
  -v fuxa_appdata:/usr/src/app/FUXA/server/_appdata \
  frangoteam/fuxa:latest

# Mosquitto MQTT
docker run -d -p 1883:1883 \
  -v $(pwd)/mosquitto/config:/mosquitto/config \
  eclipse-mosquitto:latest

# Access: http://localhost:1881
```

## MQTT Device Setup

```mermaid
sequenceDiagram
    participant FUXA
    participant Broker
    participant Device

    FUXA->>Broker: Connect (mqtt://localhost:1883)
    Broker->>FUXA: Connected ✓

    FUXA->>Broker: Subscribe: sensor/+/data

    Device->>Broker: Publish: sensor/device1/data
    Broker->>FUXA: Forward message

    Note over FUXA: Parse JSON<br/>Update tags
```

**In FUXA:**

1. Connections → Add → MQTTclient
2. Name: `mqtt_broker`
3. Broker: `mqtt://localhost:1883`
4. QoS: 0 (fastest)

## Tag Mapping

**Subscribe:** `sensor/+/data` (all devices)

**Map flat JSON fields:**

| Tag | JSON Path | Type | Description |
|-----|-----------|------|-------------|
| alarm_active | `alarm_active` | bool | Red banner visible |
| frozen | `frozen` | bool | Sampling stopped |
| idle | `idle` | bool | Motor stopped |
| cluster | `cluster` | int | Current cluster ID |
| label | `label` | string | Fault name |
| k | `k` | int | Total clusters |
| rms_avg | `rms_avg` | float | Vibration (10s avg) |
| rms_max | `rms_max` | float | Vibration (10s max) |
| peak_avg | `peak_avg` | float | Peak (10s avg) |
| crest_avg | `crest_avg` | float | Crest (10s avg) |
| buffer_samples | `buffer_samples` | int | Frozen buffer size |

## Dashboard Layout

```mermaid
graph TB
    A[Alarm Banner<br/>Red, flashing] --> B{alarm_active?}
    B -->|true| C[Show banner]
    B -->|false| D[Hide banner]

    E[Idle Banner<br/>Blue] --> F{idle?}
    F -->|true| G[Show: Motor stopped]
    F -->|false| H[Hide]

    I[Status Display] --> J[Cluster: label]
    I --> K[K: total clusters]

    L[Gauges] --> M[RMS: 0-20 m/s²]
    L --> N[Peak: 0-30 m/s²]
    L --> O[Crest: 0-5]

    P[Summary Table] --> Q[Last 20 summaries<br/>timestamp, cluster, rms]

    R[Operator Controls] --> S[Input: label name]
    R --> T[Button: Label]
    R --> U[Button: Discard]
```

## Operator Workflow

```mermaid
stateDiagram-v2
    [*] --> Monitoring: Normal operation

    Monitoring --> AlarmTrigger: RMS spike detected
    AlarmTrigger --> RedBanner: Device freezes

    RedBanner --> PhysicalCheck: Operator walks to motor

    PhysicalCheck --> MotorRunning: Check conditions
    MotorRunning --> RedBanner: Still running

    PhysicalCheck --> MotorStopped: Motor stops
    MotorStopped --> BlueBanner: Shift change

    BlueBanner --> NextShift: Wait for operator
    NextShift --> Inspect: Physical inspection

    Inspect --> Decision: What found?

    Decision --> Label: Real fault
    Label --> CreateCluster: "Type#58; 'bearing_fault'"
    CreateCluster --> Monitoring: K = K + 1

    Decision --> Discard: False alarm
    Discard --> Monitoring: K unchanged

    RedBanner --> Inspect: Immediate response
```

## Widget Configuration

**1. Alarm Banner**

```yaml
Type: Rectangle
Visibility: alarm_active == true
Background: #FF0000 (red)
Text: "⚠️ ANOMALY DETECTED - INSPECT MOTOR"
Font: Bold, 24px
Position: Top, full width
Animation: Flashing (optional)
```

**2. Idle Banner**

```yaml
Type: Rectangle
Visibility: idle == true
Background: #2196F3 (blue)
Text: "⏸️ MOTOR IDLE - Alarm Held"
Font: Normal, 18px
Position: Below alarm banner
Height: 50px
```

**3. Gauges**

```yaml
RMS Gauge:
  Min: 0
  Max: 20
  Bind: rms_avg
  Yellow: >10
  Red: >15

Peak Gauge:
  Min: 0
  Max: 30
  Bind: peak_avg
  Yellow: >15
  Red: >20

Crest Gauge:
  Min: 0
  Max: 5
  Bind: crest_avg
  Yellow: >2
  Red: >3
```

**4. Label Button**

```yaml
Type: Button
Text: "Label"
Enabled: alarm_active == true
Action: MQTT Publish
  Topic: tinyol/{device_id}/label
  Payload: {"label":"${label_input}"}
```

**5. Discard Button**

```yaml
Type: Button
Text: "Discard"
Enabled: alarm_active == true
Action: MQTT Publish
  Topic: tinyol/{device_id}/discard
  Payload: {"discard":true}
```

## Test Workflow

```mermaid
sequenceDiagram
    participant Test
    participant Broker
    participant FUXA
    participant Device

    Note over Test: 1. Normal data
    Test->>Broker: sensor/test/data<br/>{alarm_active: false, rms_avg: 5.2}
    Broker->>FUXA: Update gauges

    Note over FUXA: Green status

    Note over Test: 2. Trigger alarm
    Test->>Broker: sensor/test/data<br/>{alarm_active: true, buffer_samples: 100}
    Broker->>FUXA: Show red banner

    Note over FUXA: Red banner visible<br/>Buttons enabled

    Note over FUXA: 3. Operator labels
    FUXA->>Broker: tinyol/test/label<br/>{"label":"bearing_fault"}
    Broker->>Device: Create cluster

    Device->>Broker: sensor/test/data<br/>{cluster: 1, k: 2, alarm_active: false}
    Broker->>FUXA: Hide banner, show C1

    Note over FUXA: Green status<br/>K = 2 clusters
```

## Troubleshooting

```mermaid
graph TD
    A[Problem?] --> B{Banner not showing?}
    B -->|Yes| C[Check visibility:<br/>alarm_active == true]

    A --> D{Buttons not working?}
    D -->|Yes| E[Subscribe to tinyol/#<br/>mosquitto_sub]

    A --> F{Device not receiving?}
    F -->|Yes| G[Check Serial Monitor<br/>Should print: Labeled]

    A --> H{Gauges frozen?}
    H -->|Yes| I[Normal: 10s updates<br/>Not real-time]
```

## Manual Testing

**1. Publish normal data:**

```bash
mosquitto_pub -h localhost -t "sensor/test/data" \
  -m '{
    "alarm_active":false,
    "frozen":false,
    "idle":false,
    "cluster":0,
    "label":"normal",
    "rms_avg":5.2,
    "peak_avg":9.1,
    "crest_avg":1.75
  }'
```

**2. Trigger alarm:**

```bash
mosquitto_pub -h localhost -t "sensor/test/data" \
  -m '{
    "alarm_active":true,
    "frozen":true,
    "cluster":-1,
    "buffer_samples":100,
    "rms_avg":12.3
  }'
```

**3. Test idle:**

```bash
mosquitto_pub -h localhost -t "sensor/test/data" \
  -m '{
    "alarm_active":true,
    "frozen":true,
    "idle":true,
    "rms_avg":0.3
  }'
```

**4. Send label:**

```bash
mosquitto_pub -h localhost \
  -t "tinyol/test_device/label" \
  -m '{"label":"bearing_fault"}'
```

## Real Scenarios

### Scenario A: Immediate Response

```mermaid
graph LR
    A[2:00 PM<br/>Alarm] --> B[2:05 PM<br/>Walk to motor]
    B --> C[2:10 PM<br/>Inspect bearing]
    C --> D[2:15 PM<br/>Label fault]
    D --> E[2:16 PM<br/>Resume monitoring]
```

### Scenario B: Shift Change

```mermaid
graph LR
    A[2:00 PM<br/>Alarm] --> B[5:00 PM<br/>Shift ends]
    B --> C[5:01 PM<br/>Motor stops]
    C --> D[Blue banner]
    D --> E[6:00 PM<br/>Next shift arrives]
    E --> F[6:30 PM<br/>Inspect safely]
    F --> G[7:00 PM<br/>Label fault]
```

### Scenario C: False Alarm

```mermaid
graph LR
    A[Alarm] --> B[Inspect] --> C[Nothing wrong]
    C --> D[Click Discard]
    D --> E[Resume<br/>K unchanged]
```

## Production Config

**Security (optional):**

```bash
# Mosquitto with auth
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
allow_anonymous false
password_file /mosquitto/config/passwords

# Add user
mosquitto_passwd -c passwords device1
```

**Backup:**

```bash
# Export FUXA config
# Settings → Export → Download JSON

# Backup Mosquitto
docker cp mosquitto:/mosquitto/config backup/
```

## Next Steps

1. ✓ Run Docker containers
2. ✓ Add MQTT device
3. ✓ Map tags
4. Create dashboard widgets
5. Test with real device
6. Monitor production

**Total time:** ~30 minutes

## References

- FUXA docs: https://frangoteam.github.io/
- Mosquitto: https://mosquitto.org/
- MQTT schema: [mqtt_schema.md](../docs/mqtt_schema.md)