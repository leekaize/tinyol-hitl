# supOS-CE Integration

Unified Namespace (UNS) architecture. MQTT native. Time-series storage included.

## Setup

**Install supOS-CE:**
```bash
docker pull supos/supos-ce:latest
docker run -d -p 8080:8080 -p 1883:1883 supos/supos-ce
```

Access UI: http://localhost:8080
MQTT broker: localhost:1883

## UNS Structure

**Namespace hierarchy:**
```
enterprise/
  site/
    area/
      line/
        device/
          {device_id}/
            data        # Streaming features
            cluster     # Assigned cluster
            correction  # Human labels (subscribe)
            model       # Centroid updates (publish)
```

## Topic Schema

**Publish (MCU → supOS):**
```
Topic: enterprise/site1/area1/line1/device/esp32_001/data
QoS: 0
Payload: {"timestamp": 1234567890, "features": [0.32, -0.15, 0.08]}

Topic: enterprise/site1/area1/line1/device/esp32_001/cluster
QoS: 0
Payload: {"cluster_id": 2, "confidence": 0.87, "timestamp": 1234567890}
```

**Subscribe (supOS → MCU):**
```
Topic: enterprise/site1/area1/line1/device/esp32_001/correction
QoS: 1
Payload: {"cluster_id": 2, "label": "bearing_fault", "operator": "user_123"}
```

## Configuration

**Create device in supOS:**
1. Navigate to Modeling → Assets
2. Add device: `esp32_001`
3. Add attributes: `data`, `cluster`, `correction`, `model`
4. Enable MQTT binding for all attributes

**MCU connection:**
```c
mqtt_cfg.broker.address.uri = "mqtt://192.168.1.100:1883";
mqtt_cfg.credentials.username = "device_esp32_001";
mqtt_cfg.credentials.password = "device_token";
```

## Time-Series Storage

**Query historical data via API:**
```bash
curl http://localhost:8080/api/v1/timeseries/query \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "device": "esp32_001",
    "attribute": "data",
    "start": 1234567890,
    "end": 1234567900
  }'
```

## Free Tier Limits

- 100 tags (attributes)
- 1 device per workspace
- 7-day data retention
- 1 concurrent user

Production: Contact supOS for enterprise license.