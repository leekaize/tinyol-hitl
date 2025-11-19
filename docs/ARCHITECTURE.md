# System Architecture

Label-driven incremental clustering with freeze-on-alarm workflow. Device learns automatically, operator labels outliers only.

## Overview

**Normal operation:** Device clusters vibration patterns automatically. Publishes summary every 10s.

**Anomaly detected:** Device freezes data, operator inspects physically, labels or discards.

**No timeouts:** Operator has unlimited time to inspect motor before deciding.

---

## Feature Extraction

**Current implementation:**
3D feature vector from accelerometer:
- **RMS**: Overall vibration energy
- **Peak**: Maximum amplitude
- **Crest Factor**: Peak/RMS ratio (bearing faults spike)

**Aggregation window:** 10 seconds (100 samples @ 10 Hz)

**Published statistics per window:**
- Average: mean(rms), mean(peak), mean(crest)
- Maximum: max(rms), max(peak), max(crest)
- Sample count: 100 samples per summary

**Why aggregate:**
- Reduces MQTT traffic (1 msg/10s vs 10 msg/s)
- Operator sees trends, not noise
- Alarms persist long enough to notice

---

## Core Algorithm

### State Machine

```
NORMAL → (outlier detected) → ALARM
ALARM → (auto-freeze) → FROZEN
FROZEN → (operator labels) → NORMAL + retrain
FROZEN → (operator discards) → NORMAL
```

**NORMAL state:**
- Collects samples in ring buffer (100 samples)
- Every 10s: compute statistics, publish summary
- Check if current window is outlier
- If outlier: transition to ALARM

**ALARM state:**
- Freeze current buffer (mark immutable)
- Publish alarm message with frozen statistics
- Transition to FROZEN immediately

**FROZEN state:**
- Buffer held indefinitely
- Wait for operator MQTT command: label or discard
- No sampling (device pauses data collection)
- On label: retrain model, clear buffer, resume
- On discard: clear buffer, resume

### Idle Detection

**Problem:** Alarms trigger during operation, but operators inspect during downtime.

**Solution:** Hold alarm state after motor stops, allowing labeling at convenience.

**Detection criteria:**
- RMS < 0.5 m/s² (minimal vibration)
- Current < 0.1 A (if current sensor available)
- Consecutive 10 samples (1 second @ 10 Hz)

**State transitions:**
```
FROZEN + motor running  → operator must respond
FROZEN + motor stops    → FROZEN_IDLE (hold indefinitely)
FROZEN_IDLE + label     → NORMAL (resume)
FROZEN_IDLE + motor on  → FROZEN (back to active alarm)
```

**Real scenario:**
1. 2:00 PM - Alarm triggers during production
2. 5:00 PM - Motor stops for shift change
3. 5:01 PM - System detects idle → FROZEN_IDLE
4. 5:30 PM - Operator inspects bearing (motor off)
5. 6:00 PM - Operator labels fault
6. Next day - Motor restarts, system resumes monitoring

**Implementation:**
```c
void kmeans_update_idle(kmeans_model_t* model,
                       fixed_t rms,
                       fixed_t current);

bool kmeans_is_idle(const kmeans_model_t* model);
```

**Why this matters:** Operators work on schedules, not milliseconds. Physical inspection requires motor shutdown. Alarms must persist across shifts.

### Outlier Detection

**Method:** Distance-based threshold

```c
// Compute distance from nearest cluster
fixed_t dist = distance_to_nearest_cluster(features);

// Get cluster radius (average inertia)
fixed_t threshold = cluster_radius * 2.0f;

// Outlier if distance > 2× normal
if (dist > threshold) {
    trigger_alarm();
}
```

**Threshold tuning:** Adjustable via MQTT command
- Default: 2× cluster radius
- Conservative: 3× (fewer false alarms)
- Aggressive: 1.5× (more sensitive)

### Ring Buffer

**Size:** 100 samples (10 seconds @ 10 Hz)

**Implementation:**
```c
typedef struct {
    fixed_t samples[100][MAX_FEATURES];
    uint16_t head;
    uint16_t count;
    bool frozen;
} ring_buffer_t;
```

**Normal operation:**
- Continuously overwrites oldest samples
- Compute statistics every 100 samples

**On alarm:**
- Mark buffer as frozen
- Head pointer stops advancing
- Samples preserved until operator acts

---

## MQTT Integration

### Topic Structure

```
sensor/{device_id}/data       # Summary messages (every 10s)
sensor/{device_id}/alarm      # Alarm notifications (on outlier)
tinyol/{device_id}/label      # Operator labels (button press)
tinyol/{device_id}/discard    # Operator discards (button press)
tinyol/{device_id}/threshold  # Adjust sensitivity
```

### Flat JSON Schema

**Normal message (every 10s):**
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

**Alarm message (immediate on outlier):**
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

**Label command (operator → device):**
```json
{
  "label": "outer_race_fault"
}
```

Device uses frozen buffer as seed for new cluster.

**Discard command (operator → device):**
```json
{
  "discard": true
}
```

Device clears buffer, resumes normal operation.

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

## FUXA SCADA Integration

### Dashboard Layout

```
┌─────────────────────────────────────────┐
│  Motor Health Monitor                   │
├─────────────────────────────────────────┤
│  [ALARM BANNER - Red, flashing]         │  ← Only visible when alarm_active
│  Anomaly detected: RMS 12.3 m/s²        │
│  [Freeze] [Discard]                     │
├─────────────────────────────────────────┤
│  Current Status:                        │
│  Cluster: [0] "normal"                  │
│  K: 2 clusters                          │
│                                         │
│  Live Features (10s average):           │
│  RMS:   5.2 m/s²  [gauge]              │
│  Peak:  9.1 m/s²  [gauge]              │
│  Crest: 1.75      [gauge]              │
│                                         │
│  History (last 20 summaries):           │
│  [Table: timestamp | cluster | rms_avg] │
│                                         │
│  Frozen Data (when alarm active):       │
│  [Table: 100 samples from buffer]       │
│                                         │
│  Label Input:                           │
│  [Input: "fault_name"] [Button: Label]  │
└─────────────────────────────────────────┘
```

### Operator Workflow

**1. Normal operation:**
- Monitor gauges and table
- RMS stays around 5 m/s²
- No action needed

**2. Anomaly detected:**
- Red banner appears: "Anomaly detected"
- Gauges show frozen values (RMS 12.3 m/s²)
- Table shows 100 buffered samples
- Device stops collecting new data

**3. Operator inspects:**
- Walks to motor
- Checks bearings, listens for noise
- Checks temperature, vibration
- Takes hours if needed (no timeout)

**4. Operator decides:**
- **If real fault:** Type label "bearing_fault", click "Label"
- **If false alarm:** Click "Discard"

**5. Device response:**
- On label: Creates new cluster, retrains on buffer
- On discard: Clears buffer, resumes normal sampling
- Banner disappears, gauges update

### MQTT Configuration

**FUXA Device:**
- Broker: `localhost:1883`
- Subscribe: `sensor/#`
- QoS: 0

**Tag mapping (flat JSON):**
| Tag | JSON Path | Type |
|-----|-----------|------|
| alarm_active | `alarm_active` | bool |
| frozen | `frozen` | bool |
| cluster | `cluster` | int |
| label | `label` | string |
| rms_avg | `rms_avg` | float |
| rms_max | `rms_max` | float |
| buffer_samples | `buffer_samples` | int |

**Button actions:**
- "Label" → Publish `tinyol/{device_id}/label` with `{"label":"${input_value}"}`
- "Discard" → Publish `tinyol/{device_id}/discard` with `{"discard":true}`

---

## Performance

**Memory footprint:**
- Model: <1 KB (16 clusters × 3 features × 4 bytes)
- Ring buffer: 1.2 KB (100 samples × 3 features × 4 bytes)
- Total: ~2.5 KB RAM

**Latency:**
- Feature extraction: 0.08 ms
- Outlier detection: 0.28 ms
- MQTT publish: 10-15 ms
- Total loop: <20 ms

**MQTT traffic:**
- Normal: 1 message per 10 seconds
- Alarm: 1 additional message on detection
- Total: ~9 KB/hour (vs 3.6 MB/hour at 10 Hz)

---

## Key Design Decisions

### Why 10-second summaries?

**Not real-time critical:** Bearing faults develop over days/weeks
**Operator response time:** Humans take minutes to act
**MQTT efficiency:** 360× less traffic vs 10 Hz streaming
**Trend visibility:** Averages filter sensor noise

### Why unlimited freeze time?

**Physical inspection required:** Operator must walk to motor
**Shift changes:** May need next operator to investigate
**Documentation:** May need photos, measurements
**Pragmatic:** Can't force 5-minute deadline on factory floor

### Why flat JSON?

**SCADA compatibility:** Not all systems parse nested objects
**Tag mapping simplicity:** Direct path to values
**Debugging ease:** Easier to log and inspect
**Industry standard:** OT systems expect flat structures

---

## Phase 2: Current Sensing (Future)

**When to add:** After vibration-only baseline validated

**Hardware:**
- ZMCT103C current transformers (3× for L1, L2, L3)
- 100Ω burden resistors
- ESP32 ADC pins: 36, 39, 34

**Feature expansion:**
- Current: [0-2] = Vibration (RMS, Peak, Crest)
- New: [3-6] = I_L1, I_L2, I_L3, Imbalance

**JSON schema update:**
Add fields: `current_l1_avg`, `current_l1_max`, etc.
Increment `schema_version` to 2.

**Expected improvement:**
- Bearing faults: Minimal (vibration sufficient)
- Electrical faults: Significant (loose connections, phase imbalance)
- Rotor bar defects: High (current signature analysis)