# Architecture

Label-driven clustering. Device learns automatically. Operator labels outliers only.

## State Machine

```mermaid
stateDiagram-v2
    [*] --> NORMAL

    NORMAL --> FROZEN: Outlier detected<br/>(distance > 2× radius)

    FROZEN --> FROZEN_IDLE: Motor stops<br/>(RMS < 0.5 m/s² for 1s)
    FROZEN_IDLE --> FROZEN: Motor restarts<br/>(RMS > 0.5 m/s²)

    FROZEN --> NORMAL: Operator labels<br/>(K = K + 1)
    FROZEN_IDLE --> NORMAL: Operator labels<br/>(K = K + 1)

    FROZEN --> NORMAL: Operator discards<br/>(K unchanged)
    FROZEN_IDLE --> NORMAL: Operator discards<br/>(K unchanged)

    note left of NORMAL
        - Sample at 10 Hz
        - Publish summary every 10s
        - Check outliers
    end note

    note right of FROZEN
        - Buffer frozen (100 samples)
        - No sampling
        - Wait for operator
    end note

    note right of FROZEN_IDLE
        - Alarm persists
        - Survives shift changes
        - Label at convenience
    end note
```

## Data Flow

```mermaid
graph TB
    subgraph "Every 100ms"
        A[Read accelerometer] --> B[Extract features]
        B --> C{State?}

        C -->|NORMAL| D[Add to buffer]
        C -->|FROZEN| E[Reject update]

        D --> F{Outlier?}
        F -->|Yes| G[Trigger alarm]
        F -->|No| H[Update centroid]

        H --> I[Increment count]
        I --> J[Update inertia]
    end

    subgraph "Every 10 seconds"
        K[Compute statistics] --> L[Publish MQTT]
        L --> M[FUXA SCADA]
    end

    subgraph "Operator action"
        M --> N{Decision}
        N -->|Label| O[Create cluster]
        N -->|Discard| P[Clear buffer]

        O --> Q[K = K + 1]
        P --> Q
        Q --> R[Resume NORMAL]
    end
```

## Feature Extraction

```mermaid
graph LR
    A[3-axis raw<br/>ax, ay, az] --> B[Magnitude]

    B --> C["RMS<br/>√(x² + y² + z²)/3"]
    B --> D["Peak<br/>max(|x|, |y|, |z|)"]

    C --> E[Crest Factor<br/>peak / RMS]
    D --> E

    E --> F["3D vector<br/>[rms, peak, crest]"]
```

**Window:** 100 samples = 10 seconds @ 10 Hz

**Why these features:**
- RMS: Overall vibration energy
- Peak: Maximum amplitude (bearing impacts spike)
- Crest: Impulsiveness indicator (>2.5 = fault)

## Clustering Algorithm

```mermaid
graph TD
    A[New sample x] --> B[Find nearest cluster]

    B --> C{Distance > threshold?}
    C -->|Yes| D[Trigger alarm]
    C -->|No| E[Update centroid]

    E --> F["EMA update<br/>c_new = c_old + α(x - c_old)"]

    F --> G["α = learning_rate / (1 + 0.01 × count)"]

    G --> H[Update inertia<br/>variance within cluster]
```

**Update rule:**
```
α = base_lr / (1 + 0.01 × count)
c_new = c_old + α(x - c_old)
```

Learning rate decays as cluster matures.

## Outlier Detection

```mermaid
graph LR
    A[Sample x] --> B[Distance to nearest cluster]
    B --> C{d > 2 × radius?}

    C -->|Yes| D[OUTLIER<br/>Freeze buffer]
    C -->|No| E[NORMAL<br/>Update cluster]

    D --> F[Wait for operator]
    F -->|Label| G[New cluster from x]
    F -->|Discard| H[Clear buffer]
```

**Threshold:** 2.0× cluster radius (adjustable 1.5-5.0)

## Idle Detection

```mermaid
graph TD
    A[Every sample] --> B{RMS < 0.5 m/s²?}
    B -->|Yes| C{Current < 0.1 A?}
    B -->|No| D[Active: Reset counter]

    C -->|Yes| E[Idle counter++]
    C -->|No| D

    E --> F{Counter >= 10?}
    F -->|Yes| G[FROZEN → FROZEN_IDLE]
    F -->|No| H[Keep counting]

    D --> I{Was FROZEN_IDLE?}
    I -->|Yes| J[FROZEN_IDLE → FROZEN]
    I -->|No| K[Stay in current state]
```

**Purpose:** Alarm persists after motor stops. Operator labels during scheduled downtime.

## MQTT Schema

```mermaid
sequenceDiagram
    participant Device
    participant Broker
    participant SCADA

    Note over Device: Every 10 seconds
    Device->>Broker: sensor/{id}/data
    Note right of Device: {cluster, rms_avg, peak_avg, ...}

    Broker->>SCADA: Forward message

    Note over SCADA: Operator inspects
    SCADA->>Broker: tinyol/{id}/label
    Note left of SCADA: {label: "bearing_fault"}

    Broker->>Device: Forward command

    Note over Device: Create cluster
    Device->>Broker: sensor/{id}/data
    Note right of Device: {cluster: 1, k: 2, ...}
```

**Topics:**
- `sensor/{id}/data` - Summary (device → SCADA)
- `tinyol/{id}/label` - Create cluster (SCADA → device)
- `tinyol/{id}/discard` - Clear buffer (SCADA → device)

## JSON Format

**Normal summary:**
```json
{
  "device_id": "tinyol_device1",
  "cluster": 0,
  "label": "normal",
  "k": 1,
  "alarm_active": false,
  "frozen": false,
  "idle": false,
  "rms_avg": 5.2,
  "rms_max": 6.1,
  "peak_avg": 9.1,
  "peak_max": 11.2,
  "crest_avg": 1.75,
  "buffer_samples": 0
}
```

**Alarm state:**
```json
{
  "alarm_active": true,
  "frozen": true,
  "cluster": -1,
  "label": "unknown",
  "buffer_samples": 100
}
```

**Idle state:**
```json
{
  "alarm_active": true,
  "frozen": true,
  "idle": true,
  "rms_avg": 0.3
}
```

## Memory Layout

```mermaid
graph TD
    A[Model: ~2.5 KB] --> B[Clusters: ~1 KB]
    A --> C[Buffer: 1.2 KB]
    A --> D[Metadata: ~300 B]

    B --> E[16 clusters × 3 features × 4 bytes]
    C --> F[100 samples × 3 features × 4 bytes]
```

**Breakdown:**
- Clusters: K × D × 4 bytes (max 16 × 64 = 4.2 KB)
- Buffer: 100 × D × 4 bytes (100 × 3 = 1.2 KB)
- Total: <2.5 KB for typical config

## Decision Tree

```mermaid
graph TD
    A[Sample arrives] --> B{Frozen?}
    B -->|Yes| C[Reject]
    B -->|No| D{Buffer >= 10?}

    D -->|No| E[Add to buffer]
    D -->|Yes| F{Outlier?}

    F -->|Yes| G[Freeze + Alarm]
    F -->|No| H[Update cluster]

    H --> I[Publish summary?]
    I -->|Every 10s| J[MQTT publish]
    I -->|No| K[Continue]
```

## Platform Abstraction

```mermaid
graph LR
    A[Core algorithm<br/>streaming_kmeans.c] --> B[Platform layer<br/>core.ino]

    B --> C[ESP32<br/>Xtensa LX6]
    B --> D[RP2350<br/>ARM Cortex-M33]

    C --> E[Wire.begin<br/>GPIO 21/22]
    D --> F[Wire.begin<br/>GPIO 4/5]

    E --> G[WiFi + MQTT]
    F --> G
```

**Same algorithm, different I/O.**

## Performance

| Metric | Value |
|--------|-------|
| Loop latency | <20 ms |
| Feature extraction | 0.08 ms |
| Outlier check | 0.28 ms |
| MQTT publish | 10-15 ms |
| Memory | 2.5 KB |
| Traffic | 9 KB/hour |

## Operator Workflow

```mermaid
graph TD
    A[Monitor SCADA] --> B{Alarm?}
    B -->|No| A
    B -->|Yes| C[Walk to motor]

    C --> D[Physical inspection]
    D --> E{Real fault?}

    E -->|Yes| F[Return to SCADA]
    F --> G[Type label]
    G --> H[Click Label]

    E -->|No| I[Click Discard]

    H --> J[Device creates cluster]
    I --> J
    J --> K[Resume monitoring]
```

**Key insight:** No timeout. Operator has unlimited time.

## Why This Works

**Traditional approach:**
1. Collect months of data
2. Label every sample
3. Train model
4. Deploy
5. Model is frozen

**TinyOL-HITL:**
1. Deploy K=1
2. Discover fault
3. Operator labels
4. K grows
5. Model adapts

**Result:** Days to deployment vs months.