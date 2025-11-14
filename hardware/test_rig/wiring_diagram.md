# Wiring Diagram

Sensor connections for ESP32 and RP2350 platforms.

## Power Supply
```
AC Source → VFD → Motor (U, V, W phases)
             ↓
          Ground
```

## Accelerometer (I²C)

### ADXL345 Pinout

**DOIT ESP32 DEVKIT V1:**
```
ADXL345          ESP32
--------         --------
VCC      →       3.3V
GND      →       GND
SDA      →       GPIO21 (SDA)
SCL      →       GPIO22 (SCL)
CS       →       3.3V (I²C mode)
```

**RP2350 (Pico 2 W):**
```
ADXL345          Pico 2 W
--------         --------
VCC      →       3V3 (Pin 36)
GND      →       GND (Pin 38)
SDA      →       GP4 (Pin 6)
SCL      →       GP5 (Pin 7)
CS       →       3V3 (I²C mode)
```

## I²C Pull-up Resistors

**Required:** 4.7kΩ pull-ups on SDA and SCL if not onboard.
```
        3.3V
         │
         ├── 4.7kΩ ──┬── SDA
         │           │
         └── 4.7kΩ ──┴── SCL
```

## Motor Control

**VFD Connections:**
```
VFD Output       Motor
----------       -----
U (R)     →      U (Red)
V (S)     →      V (Yellow)
W (T)     →      W (Blue)
PE        →      Ground (Green/Yellow)
```

**Control signals:**
- START/STOP: Digital input (24V)
- Speed reference: 0-10V analog or Modbus
- Emergency stop: NC contact (normally closed)

## Emergency Stop Wiring
```
24V → E-Stop Button (NC) → VFD STOP Input → GND
```

Press E-Stop → Opens circuit → VFD stops motor immediately.

## Platform-Specific Notes

**ESP32:**
- GPIO21/22 are default I²C pins
- WiFi antenna keeps clear of motor (EMI)
- Power via USB or 5V external supply

**RP2350:**
- GP4/5 are default I²C pins
- CYW43 WiFi chip onboard
- Power via USB or VSYS (5V)

## Shielding

**Sensor cable:** Twisted pair with shield
```
Shield → Ground at one end only (star ground)
SDA/SCL → Twisted pair inside shield
```

**Ground loop prevention:** Connect shield at sensor end, leave floating at MCU end.

## Calibration Setup

**Vibration calibrator:** 10 Hz, 1g reference
```
ADXL345 → Mount on calibrator → Verify ±5% of 1g output
```

**Tachometer:** Optical tachometer to verify RPM
```
Reflective tape on shaft → Measure actual RPM → Compare to VFD display (±2%)
```

## Troubleshooting

**Sensor reads zero:** Check I²C connection, verify power supply.
**Excessive noise:** Ground loop? Use twisted pair. Separate power grounds.
**No fault signature:** Defect too small? Increase depth. Recheck geometry.
**Motor won't start:** VFD config? Check phase wiring. Verify voltage.