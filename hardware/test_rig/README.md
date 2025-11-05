# Hardware Test Rig

Induction motor with real bearing faults. Tri-platform validation.

## Motor
**Power:** 0.5 HP / 370W
**Speed:** 1000-3000 RPM (VFD controlled)
**Bearing:** 6203-2RS deep groove ball

## Sensor
**Accelerometer:** ADXL345 (I²C) or MPU6050
**Placement:** Motor housing, drive-end bearing
**Sampling:** 12 kHz (matches CWRU dataset)

## Platforms Tested
1. **ESP32-S3:** WiFi, MQTT to SCADA (primary)
2. **RP2350 (Pico 2 W):** WiFi, power comparison

## Operating Conditions
See `operating_conditions.md` for fault scenarios.

## Wiring
See `wiring_diagram.md` for sensor connections.