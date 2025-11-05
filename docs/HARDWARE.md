# Hardware Test Rig

Induction motor setup for generating real bearing faults. Validate algorithm on actual hardware.

## Motor Specifications

**Model:** [Document your specific motor]

**Rated power:** [e.g., 0.5 HP / 370 W]

**Speed range:** [e.g., 0-3000 RPM via VFD]

**Bearing type:** [e.g., 6203-2RS deep groove ball bearing]

**Mounting:** [e.g., horizontal, foot-mounted]

## Sensor Setup

**Accelerometer:** ADXL345 or MPU6050 (I²C interface)

**Placement:** Motor housing, near drive-end bearing

**Mounting method:** Stud mount or epoxy adhesive

**Sampling rate:** 12 kHz (matches CWRU dataset)

**Resolution:** 13-bit (±16g range)

## Operating Conditions

Generate distinguishable vibration signatures. Start healthy. Add faults incrementally.

### Baseline (Healthy)

**Purpose:** Establish normal vibration profile.

**Configuration:**
- Clean bearings, no defects
- Aligned coupling (within 0.05 mm)
- Balanced rotor (ISO G6.3)

**Test speeds:** 1000, 1500, 2000 RPM

**Duration:** 5 minutes per speed

**Data collection:** Record acceleration in 3 axes

### Imbalance

**Purpose:** Simulate rotor imbalance (common fault).

**Configuration:**
- Add 10g mass to shaft at 0°, 90°, 180°, 270° positions
- Test each position separately

**Test speeds:** 1000, 1500, 2000 RPM

**Expected signature:** 1× RPM frequency spike

**Severity levels:**
- Mild: 10g at 50 mm radius
- Moderate: 20g at 50 mm radius
- Severe: 30g at 50 mm radius

### Misalignment

**Purpose:** Simulate coupling misalignment.

**Configuration:**
- Parallel: Offset shafts by 0.5 mm, 1.0 mm, 1.5 mm
- Angular: Rotate motor mount by 0.5°, 1.0°, 1.5°

**Test speeds:** 1000, 1500, 2000 RPM

**Expected signature:** 2× RPM frequency spike

### Bearing Faults

**Purpose:** Validate algorithm on target fault type.

**Configuration:**
- **Ball defect:** 0.5 mm notch on single ball
- **Inner race:** 0.5 mm notch on raceway
- **Outer race:** 0.5 mm notch on raceway

**Bearing geometry (6203-2RS typical):**
- Pitch diameter: 29 mm
- Ball diameter: 7 mm
- Number of balls: 8
- Contact angle: 0° (radial)

**Fault frequencies:**
- BPFO (ball pass outer): 4.95 × RPM / 60
- BPFI (ball pass inner): 7.05 × RPM / 60
- BSF (ball spin): 2.36 × RPM / 60
- FTF (cage): 0.38 × RPM / 60

**Test speeds:** 1000, 1500, 2000 RPM

**Expected signature:** Spikes at calculated fault frequencies

**Safety:** Bearing faults create vibration. Monitor for excessive levels. Stop if >10 mm/s RMS.

### Electrical Faults

**Purpose:** Broaden fault coverage.

**Configuration:**
- Single-phase operation: Disconnect one phase
- Voltage unbalance: Reduce one phase by 10%, 20%

**Test speeds:** 1000, 1500 RPM only (reduced load)

**Expected signature:** 2× line frequency sidebands

## Wiring Diagram

**Power:**
```
AC Source → VFD → Motor (U, V, W phases)
             ↓
          Ground
```

**Data acquisition:**
```
Accelerometer → RP2350 / ESP32
   (I²C)           (GPIO pins)
                      ↓
                   USB Serial
                      ↓
                   Computer
```

**Pinout (RP2350):**
- SDA: GP4
- SCL: GP5
- GND: GND
- VCC: 3V3

**Pinout (ESP32):**
- SDA: GPIO21
- SCL: GPIO22
- GND: GND
- VCC: 3.3V

## Data Collection Protocol

**Per condition:**
1. Set speed via VFD
2. Wait 60 seconds for stabilization
3. Record 60 seconds of data
4. Save with label: [condition]_[speed]_[trial].bin

**Labeling scheme:**
```
healthy_1000_01.bin
imbalance_10g_0deg_1500_01.bin
bearing_outer_1500_01.bin
```

**Trials:** Minimum 3 per condition. Check repeatability.

**Verification:** FFT each trial. Peak frequencies match theory? If not, recheck setup.

## Safety

**Hazards:**
- Rotating shaft (entanglement)
- Bearing failure (projectile parts)
- Electrical shock (VFD output)

**Mitigation:**
- Guard around rotating parts
- Emergency stop button within reach
- Ground all equipment
- Eye protection during fault tests
- Never exceed 3000 RPM

**Wear limits:** Replace bearings after 10 fault tests. Structural damage accumulates.

## Calibration

**Accelerometer:**
1. Place sensor on vibration calibrator (10 Hz, 1g)
2. Record output. Verify ±5% of expected.
3. Apply correction factor if needed.

**Tachometer:**
1. Attach reflective tape to shaft
2. Verify RPM with optical tachometer
3. VFD display vs actual RPM should match ±2%

**Frequency validation:**
1. Record healthy baseline
2. FFT, identify RPM peak
3. Compare calculated fault frequencies to bearing spec
4. If mismatch >5%, recheck geometry

## Equipment List

- [ ] Induction motor (0.5-2 HP)
- [ ] Variable frequency drive (VFD)
- [ ] ADXL345 or MPU6050 accelerometer
- [ ] RP2350 or ESP32 dev board
- [ ] Power supply (5V, 2A for MCU)
- [ ] USB cable (data logging)
- [ ] Motor mount with alignment capability
- [ ] Mass set (10g, 20g, 30g)
- [ ] Hand tools (wrenches, screwdrivers)
- [ ] Safety guard around motor
- [ ] Emergency stop button
- [ ] Optical tachometer
- [ ] Vibration calibrator (optional, for accuracy)

## Expected Results

**Healthy:** Low amplitude, smooth spectrum

**Imbalance:** 1× RPM spike dominant

**Misalignment:** 2× RPM spike dominant

**Bearing fault:** Spikes at BPFO/BPFI/BSF frequencies

**Streaming k-means:** Should form distinct clusters per condition. HITL corrections improve separation.

## Troubleshooting

**Sensor reads zero:** Check I²C connection. Verify power supply.

**Excessive noise:** Ground loop? Use twisted pair for I²C. Separate power grounds.

**No fault signature:** Defect too small? Increase depth. Recheck bearing geometry.

**Motor won't start:** VFD configuration? Check phase wiring. Verify voltage.

## Next Steps

1. Document your specific motor model and bearing type
2. Build test rig, verify healthy baseline
3. Generate one fault condition, validate signature
4. Automate data collection script
5. Stream to MCU, test clustering
6. Inject HITL labels, measure improvement