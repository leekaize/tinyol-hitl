# Hardware Test Rig

Real motor. Real faults. Real validation. Three platforms tested with identical setup.

## Goal

Generate distinguishable vibration signatures. Validate streaming k-means against physical reality. Compare ESP32 and RP2350 performance on identical hardware.

## Motor Specifications

**Power:** 0.5 HP / 370W

**Speed range:** 1000-3000 RPM (VFD controlled)

**Bearing type:** 6203-2RS deep groove ball bearing

**Mounting:** Horizontal, foot-mounted

**Why this size:** Small enough for lab. Large enough for measurable faults.

## Sensor Setup

**Accelerometer:** ADXL345 (recommended) or MPU6050

**Interface:** I²C (4 wires: VCC, GND, SDA, SCL)

**Placement:** Motor housing, near drive-end bearing

**Sampling rate:** 12 kHz (matches CWRU dataset)

**Resolution:** 13-bit, ±16g range

**Why ADXL345:** Proven in literature. I²C = simple wiring. Arduino libraries exist.

## Arduino IDE Platform Configuration

**Auto-detection via config.h:**
```cpp
// Detects board and sets pins automatically
#if defined(ARDUINO_ARCH_ESP32)
  #define PLATFORM_ESP32
  #define HAS_WIFI
  // I²C: GPIO21 (SDA), GPIO22 (SCL)
#elif defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #define PLATFORM_RP2350
  #define HAS_WIFI
  // I²C: GP4 (SDA), GP5 (SCL)
```

**One sketch. Three platforms. Zero manual pin changes.**

## Wiring Diagrams

### Power System
```
AC Source → VFD → Motor (U, V, W)
             ↓
          Ground
```

### Sensor Connections (All Platforms)

**ESP32-S3:**
```
ADXL345          ESP32-S3
--------         --------
VCC      →       3.3V
GND      →       GND
SDA      →       GPIO21 (default I²C SDA)
SCL      →       GPIO22 (default I²C SCL)
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

**Critical:** ADXL345 is 3.3V device. Arduino runs 5V. Use level shifter or 3.3V supply.

### I²C Pull-up Resistors
```
        3.3V
         │
    4.7kΩ├─────┬─── SDA
         │     │
    4.7kΩ└─────┴─── SCL
```

**Note:** Most dev boards have onboard pull-ups. Add external only if bus unstable.

## Arduino Sketch Integration

**Libraries required:**
```cpp
#include <Wire.h>           // I²C communication
#include <Adafruit_ADXL345_U.h>  // ADXL345 driver
```

**Setup in core.ino:**
```cpp
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

void setup() {
  Serial.begin(115200);

  if (!accel.begin()) {
    Serial.println("ADXL345 not found!");
    while (1) platform_blink(10);
  }

  accel.setRange(ADXL345_RANGE_16_G);
  accel.setDataRate(ADXL345_DATARATE_3200_HZ);

  // Initialize k-means...
}
```

**Reading in loop():**
```cpp
void loop() {
  sensors_event_t event;
  accel.getEvent(&event);

  // Convert to fixed-point
  fixed_t features[3] = {
    FLOAT_TO_FIXED(event.acceleration.x),
    FLOAT_TO_FIXED(event.acceleration.y),
    FLOAT_TO_FIXED(event.acceleration.z)
  };

  uint8_t cluster = kmeans_update(&model, features);

  // MQTT publish if WiFi available
  #ifdef HAS_WIFI
  mqtt.publishCluster(cluster, features, 3);
  #endif
}
```

## Operating Conditions (Simplified for MVP)

### Phase 1: Baseline (Healthy)
**Goal:** Establish normal vibration profile.

**Configuration:** Clean bearings, aligned coupling, balanced rotor

**Test speeds:** 1500 RPM only (simplify for MVP)

**Duration:** 5 minutes

**Success:** Cluster formation visible in Serial Monitor

### Phase 2: Single Fault (Outer Race)
**Goal:** Validate fault detection.

**Configuration:** 0.5 mm notch on outer race

**Test speed:** 1500 RPM

**Expected:** New cluster forms, distinct from baseline

**Success:** >80% samples assigned to new cluster

### Phase 3: HITL Correction
**Goal:** Test human-in-the-loop.

**Method:** Send MQTT correction for misclassified samples

**Measure:** Accuracy improvement after 10% labels

**Target:** +15% accuracy vs baseline

**Priority:** Phases 1-2 sufficient for MVP. Phase 3 if time permits.

## Bearing Fault Frequencies (6203-2RS)

**Geometry:**
- Pitch diameter: 29 mm
- Ball diameter: 7 mm
- Number of balls: 8
- Contact angle: 0°

**Fault frequencies @ 1500 RPM (25 Hz):**
- BPFO (ball pass outer): 123.75 Hz
- BPFI (ball pass inner): 176.25 Hz
- BSF (ball spin): 59 Hz
- FTF (cage): 9.5 Hz

**Verification:** FFT should show spikes at these frequencies.

## Data Collection Protocol (MVP)

**Per condition:**
1. Set VFD to 1500 RPM
2. Wait 60 seconds (stabilization)
3. Record 60 seconds
4. Upload sketch, watch Serial Monitor
5. Save output to .txt file

**Labeling:**
```
healthy_1500_01.txt
outer_race_1500_01.txt
```

**Trials:** 3 minimum per condition

**Verification:** Check Serial output for cluster assignment consistency

## Safety (Critical)

**Hazards:**
- Rotating shaft → entanglement
- Bearing failure → projectile parts
- Electrical shock → VFD output

**Mitigation:**
- Guard around motor (mandatory)
- E-stop button within reach
- Ground all equipment
- Eye protection during tests
- Never exceed 3000 RPM
- Replace bearings after 10 fault tests

**If anything looks unsafe, stop immediately.**

## Calibration Steps

**Accelerometer:**
1. Place sensor horizontal, still
2. Z-axis should read ~1g (gravity)
3. X/Y axes should read ~0g
4. If not: check sensor orientation

**Tachometer:**
1. Attach reflective tape to shaft
2. Use optical tachometer
3. Verify VFD display matches actual RPM (±2%)

**Frequency validation:**
1. Record healthy baseline
2. FFT in Python/MATLAB
3. Identify RPM peak (should be 25 Hz @ 1500 RPM)
4. Compare to calculated fault frequencies

## Equipment Checklist

**Essential (MVP):**
- [ ] Induction motor (0.5-2 HP)
- [ ] VFD
- [ ] ADXL345 accelerometer
- [ ] ESP32-S3 or RP2350 dev board
- [ ] Power supply (5V, 2A for MCU)
- [ ] USB cable
- [ ] Safety guard around motor
- [ ] E-stop button

**Nice to have:**
- [ ] Second dev board (platform comparison)
- [ ] Optical tachometer (RPM verification)
- [ ] Vibration calibrator (accuracy validation)

## Platform Comparison Testing

**Method:**
1. Run identical test on ESP32
2. Run identical test on RP2350

**Metrics:**
- Cluster formation speed
- MQTT publish rate (WiFi platforms)
- Serial output latency
- Power consumption (see POWER.md)

**Success:** All platforms form clusters. WiFi platforms report to SCADA.

## Troubleshooting

**Sensor reads zero:**
- Check I²C wiring
- Verify 3.3V supply
- Run I²C scanner sketch

**Excessive noise:**
- Ground loop? Separate grounds.
- EMI from motor? Use shielded cable.
- Move MCU away from motor.

**No fault signature:**
- Defect too small? Increase depth.
- Wrong frequency? Recheck geometry.
- Speed incorrect? Verify tachometer.

**Motor won't start:**
- VFD configuration? Check manual.
- Phase wiring? Verify U, V, W.
- Emergency stop pressed? Reset.

## Next Steps (Priority Order)

1. **Wire one platform** - ESP32 or RP2350 (WiFi simplifies testing)
2. **Test baseline** - Healthy motor, verify clustering
3. **Induce one fault** - Outer race defect
4. **Validate separation** - New cluster forms?
5. **If time:** Second platform comparison
6. **If time:** HITL corrections via MQTT

**Ship working code on one platform. Expand coverage later.**

## Arduino IDE Upload Settings

**ESP32:**
- Board: ESP32S3 Dev Module
- Port: /dev/ttyUSB0 (Linux) or COM3 (Windows)
- Upload Speed: 921600

**RP2350:**
- Board: Raspberry Pi Pico 2 W
- Port: /dev/ttyACM0 (Linux) or COM4 (Windows)
- Upload Speed: default

**Verification:** Upload blink sketch first. LED works? Proceed to sensor test.

**Remember:** One wiring diagram. One sketch. Three platforms. Zero manual configuration.

## Supported Sensors

### Accelerometers
**GY-521 (MPU6050)** - Primary, readily available
- Range: ±2g to ±16g (configurable)
- Interface: I²C (0x68 / 0x69)
- Library: `MPU6050_light`
- Power: 0.5 mA (accel-only mode)

**ADXL345** - Alternative, documentation reference
- Range: ±2g to ±16g
- Interface: I²C (0x53 / 0x1D)
- Library: `Adafruit_ADXL345_U`
- Power: 0.14 mA

### Current Sensors (Phase 2)
**ACS712** - Hall effect, analog
- Variants: 5A / 20A / 30A
- Interface: Analog ADC
- Use case: 0.5 HP motor → ACS712-05A

**INA219** - Shunt-based, digital
- Range: ±3.2A
- Interface: I²C (0x40)
- Use case: Precision current monitoring

## Sensor Selection in config.h
```cpp
#define SENSOR_TYPE SENSOR_MPU6050  // or SENSOR_ADXL345
#define HAS_CURRENT_SENSOR false    // true when current sensor added
```

## Integration Checklist

**Phase 1: Accelerometer Working (Days 1-2)**
- [ ] GY-521 wired to ESP32 (VCC, GND, SDA, SCL)
- [ ] I²C scan detects 0x68
- [ ] Read 3-axis at 100 Hz
- [ ] Gravity test passes (Z ≈ 1g)
- [ ] ADXL345 support compiles (framework proof)

**Phase 2: Feature Extraction (Days 2-3)**
- [ ] Buffer 256 samples (21ms window @ 12 kHz)
- [ ] Compute RMS, kurtosis, crest, variance
- [ ] Output 4D feature vector
- [ ] Match CWRU feature magnitudes

**Phase 3: Current Monitoring (Week 2, optional)**
- [ ] ACS712 wired to ADC pin
- [ ] RMS current calculation (window-based)
- [ ] Feature vector expands to 5D: [accel_x, accel_y, accel_z, accel_features, current_rms]
- [ ] Correlate current spikes with vibration

**Phase 4: CWRU Validation (Week 2)**
- [ ] Stream dataset via Serial
- [ ] Run Phase 1/2/3 experiments
- [ ] Generate confusion matrices