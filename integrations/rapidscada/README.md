# RapidSCADA Integration

Open-source SCADA. Modbus RTU/TCP and OPC-UA support. Windows/Linux deployment.

## Installation

**Linux:**
```bash
wget https://rapidscada.org/download/RapidScada_6.0.tar.gz
tar -xzf RapidScada_6.0.tar.gz
cd RapidScada
sudo ./install.sh
```

Access: http://localhost:10008
Default: admin / 12345

## Architecture

**Components:**
- SCADA Server: Data processing, alarm logic
- SCADA Comm: Device drivers (Modbus, OPC-UA, MQTT)
- Web Station: Operator HMI

## MQTT Driver

**Install Communicator MQTT plugin:**
```bash
cd ScadaComm/Drivers
wget https://rapidscada.org/download/KpMqtt_6.0.zip
unzip KpMqtt_6.0.zip
```

**Configure device:**
1. Open Communicator settings
2. Add device → MQTT Client
3. Broker: localhost:1883
4. Topics: sensor/+/data, sensor/+/cluster
5. QoS: 0 for data, 1 for corrections

## Modbus RTU (Direct Sensor)**

Alternative to WiFi. RS485 wired connection.

**Wiring:**
- MCU TX → MAX485 DI
- MCU RX → MAX485 RO
- A/B terminals → RapidSCADA RTU adapter

**Modbus holding registers:**
- 40001-40003: Feature vector (3 floats as 6 regs)
- 40004: Cluster ID (uint16)

**MCU code:**
```c
// Respond to Modbus read (function code 0x03)
modbus_set_holding_register(40001, feature[0]);
modbus_set_holding_register(40002, feature[1]);
modbus_set_holding_register(40003, feature[2]);
modbus_set_holding_register(40004, cluster_id);
```

**RapidSCADA config:**
```xml
<Device>
  <Address>1</Address>
  <Protocol>ModbusRTU</Protocol>
  <Port>COM3</Port>
  <BaudRate>115200</BaudRate>
  <Registers>
    <Holding>40001-40004</Holding>
  </Registers>
</Device>
```

## OPC-UA (Industrial Integration)

Bridge to existing PLCs/DCS systems.

**Setup OPC-UA server:**
1. Install KpOpcUa driver
2. Configure endpoint: opc.tcp://localhost:4840
3. Map MQTT topics to OPC-UA nodes

**Node structure:**
```
Objects/
  TinyOL/
    Device_ESP32_001/
      Data (Array[Float])
      ClusterID (UInt16)
      Correction (String)
```

## Web HMI

**Display real-time clusters:**
1. Create scheme: Schemes → Add → Cluster Monitor
2. Add dynamic text: bind to ClusterID tag
3. Add chart: bind to Data tag, 60s history
4. Add button: "Correct Label" → write to Correction tag

**JavaScript for label correction:**
```javascript
function correctLabel(clusterId, newLabel) {
  scada.sendCommand(
    "Correction",
    `{"cluster":${clusterId},"label":"${newLabel}"}`
  );
}
```

## Production Checklist

- [ ] MQTT TLS enabled
- [ ] Device authentication configured
- [ ] Tag ACL per device ID
- [ ] Backup PostgreSQL database
- [ ] Alarm on connection loss
- [ ] Log label corrections to audit trail