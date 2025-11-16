# Full System Build

## Complete Parts List (Phase 1)

| # | Item | Qty | Unit (RM) | Total | Got? | Notes |
|---|------|-----|-----------|-------|------|-------|
| **MICROCONTROLLERS** |
| 1 | DOIT ESP32 DEVKIT V1 | 2 | 30 | 60 | ✔ | Primary platform |
| 2 | RP2040 Pico 2W | 2 | 25 | 50 | ✔ | Cross-platform validation |
| **VIBRATION SENSORS** |
| 3 | MPU6050 (GY-521) | 2 | 12 | 24 | ✔ | Phase 1 sensor |
| 4 | ADXL345 breakout | 2 | 20 | 40 | ✘ (Arriving) | Phase 1 backup |
| **MOTOR SYSTEM** |
| 5 | 0.5 HP 3-phase motor | 1 | 250 | 250 | ✔ | 1500 RPM, 6203 bearing |
| 6 | VFD 0.5-1 HP | 1 | 400 | 400 | ✔ | 0-60Hz, 3-phase output |
| 7 | 3-phase power cable 3m | 1 | 30 | 30 | ✔ | VFD to motor |
| 8 | Motor mounting plate | 1 | 50 | 50 | ✔ | Or use existing bench |
| **WIRING & CONNECTIONS** |
| 9 | Breadboard | 3 | 10 | 30 | ✔ | MCU circuits |
| 10 | Micro USB cables | 4 | 8 | 32 | ✔ | 2 for MCUs + 2 spares |
| 11 | Dupont jumper wires pack | 2 | 8 | 16 | ✔ | Male-female, 40pcs |
| 12 | 4.7kΩ resistors | 10 | 0.5 | 5 | ✔ | I²C pull-ups |
| **MOUNTING** |
| 13 | Double-sided tape | 1 | 8 | 8 | ✔ | Quick attach |
| **POWER** |
| 14 | 5V 2A USB adapter | 3 | 15 | 45 | ✔ | MCU power |

**Phase 1 Total:** ~1040 RM

---

## Phase 2: Current Sensing (Deferred)

| # | Item | Qty | Unit (RM) | Total | Notes |
|---|------|-----|-----------|-------|-------|
| **CURRENT MONITORING** |
| 15 | ZMCT103C CT sensor 5A | 3 | 15 | 45 | One per phase |
| 16 | Burden resistor 100Ω 1W | 3 | 2 | 6 | CT output conditioning |

**Phase 2 Total:** +51 RM

**Why deferred:**
- Need vibration-only baseline first
- Paper compares 3D vs 7D features
- Current sensing adds complexity without proving core algorithm

---

## Research-Specific Redundancy

**2 MCU types:** ESP32 (Xtensa) + RP2040 (ARM) = validates portability

**2 sensor types:** MPU6050 + ADXL345 = eliminates sensor bias

**Critical:** If one MCU/sensor fails, research continues.

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