# Full System Build

## Complete Parts List (Tracking Table)

| # | Item | Qty | Unit Price (RM) | Total (RM) | Got? | Notes |
|---|------|-----|-----------------|------------|------|-------|
| **MICROCONTROLLERS** |
| 1 | DOIT ESP32 DEVKIT V1 | 2 | 30 | 60 | ✔ | Primary platform |
| 2 | RP2350 (Pico 2 W) | 2 | 25 | 50 | ✔ | Cross-platform validation |
| **VIBRATION SENSORS** |
| 3 | GY-521 (MPU6050) | 2 | 12 | 24 | ✔ | Sensor A |
| 4 | ADXL345 breakout | 2 | 20 | 40 | ✘ (Ordered) | Sensor B |
| **CURRENT MONITORING** |
| 5 | ZMCT103C CT sensor 5A | 3 | 15 | 45 | ✘ (Ordered) | One per motor phase |
| 6 | Burden resistor 100Ω 1W | 3 | 2 | 6 | ✔ | For CT output conditioning |
| **MOTOR SYSTEM** |
| 7 | 0.5 HP 3-phase motor | 1 | 250 | 250 | ✔ | 1500 RPM, 6203 bearing |
| 8 | VFD 0.5-1 HP | 1 | 400 | 400 | ✔ | 0-60Hz, 3-phase output |
| 9 | 3-phase power cable 3m | 1 | 30 | 30 | ✔ | VFD to motor |
| 10 | Motor mounting plate | 1 | 50 | 50 | ✔ | Or use existing bench |
| **WIRING & CONNECTIONS** |
| 11 | Breadboard | 3 | 10 | 30 | ✔ | MCU + sensor circuits |
| 12 | Micro USB cables | 4 | 8 | 32 | ✔ | 2 for MCUs + 2 spares |
| 13 | Dupont jumper wires pack | 2 | 8 | 16 | ✔ | Male-female, 40pcs each |
| 14 | 4.7kΩ resistors | 10 | 0.5 | 5 | ✔ | I²C pull-ups |
| **MOUNTING** |
| 15 | Double-sided mounting tape | 1 | 8 | 8 | ✔ | Quick attach |
| **POWER SUPPLIES** |
| 16 | 5V 2A USB adapter | 3 | 15 | 45 | ✔ | MCU power (have spares) |

---

## Research-Specific Redundancy Summary

**2 MCU types:** ESP32-S3 (Xtensa) + RP2350 (ARM) = validates architecture portability

**2 sensor types:** MPU6050 + ADXL345 = eliminates sensor-specific artifacts

**3 current sensors:** One per motor phase = detects phase imbalance

**Critical:** If one MCU/sensor fails, you still have valid data. Research doesn't stop.

---

## Current Sensing Wiring

```
Motor Phase L1 → ZMCT103C clamp → Burden resistor (100Ω) → ESP32 ADC (GPIO34)
                                  ↓
                                 GND
```

**ADC reading formula:**
```
I_rms = (ADC_voltage - 2.5V) / (100Ω × turns_ratio)
```

ZMCT103C turns ratio = 1000:1
5A primary → 5mA secondary → 0.5V across 100Ω

**Add to features array:** `[accel_x, accel_y, accel_z, accel_rms, current_L1, current_L2, current_L3]` = 7D feature vector

## Fault Simulation

- Different speed on motor
- Eccentric weight on pulley (non-destructive)

### Eccentric Weight Testing
**Goal:** Validate vibration pattern clustering

**Configurations:**
1. **Baseline:** No weights, balanced rotor
2. **Mild unbalance:** 50 g·mm (1 bolt at 20mm)
3. **Moderate unbalance:** 100 g·mm (2 bolts)
4. **Severe unbalance:** 200 g·mm (4 bolts)

**Test protocol:**
1. Mount pulley on motor shaft
2. Run 1500 RPM, record 60s per configuration
3. Add weights incrementally
4. Verify new cluster forms for each condition

**Expected:** 3-4 distinct clusters (healthy + 3 unbalance levels)

**Success:** >70% samples in correct cluster per condition

### Unbalance Vibration Signature

**Fundamental frequency:** Equals shaft speed (1500 RPM = 25 Hz)

**Harmonics:** 2× (50 Hz), 3× (75 Hz) for severe unbalance

**Amplitude scaling:** Proportional to unbalance mass × radius

**Verification:** FFT should show peak at shaft frequency, amplitude increases with weight