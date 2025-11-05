# SCADA Integrations

Connect to open-source industrial systems via MQTT.

## Architecture
```
MCU → WiFi → MQTT Broker → SCADA → Web HMI
                             ↓
                        Label Corrections
```

## Supported Systems

**supOS-CE:** Unified Namespace, MQTT native
**RapidSCADA:** Modbus/OPC-UA bridge, open-source

## Topic Schema
```
sensor/{device_id}/data       # Streaming features
sensor/{device_id}/cluster    # Assigned cluster
sensor/{device_id}/correction # Human labels (subscribe)
sensor/{device_id}/model      # Centroid updates
```

See `supos/` and `rapidscada/` for setup guides.