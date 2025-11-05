# Operating Conditions

Generate distinguishable vibration signatures. Start healthy. Add faults incrementally.

## Baseline (Healthy)

**Purpose:** Establish normal vibration profile.

**Configuration:**
- Clean bearings, no defects
- Aligned coupling (within 0.05 mm)
- Balanced rotor (ISO G6.3)

**Test speeds:** 1000, 1500, 2000 RPM
**Duration:** 5 minutes per speed
**Data collection:** Record acceleration in 3 axes

## Imbalance

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

## Misalignment

**Purpose:** Simulate coupling misalignment.

**Configuration:**
- **Parallel:** Offset shafts by 0.5 mm, 1.0 mm, 1.5 mm
- **Angular:** Rotate motor mount by 0.5°, 1.0°, 1.5°

**Test speeds:** 1000, 1500, 2000 RPM

**Expected signature:** 2× RPM frequency spike

## Bearing Faults

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

**Safety:** Monitor vibration. Stop if >10 mm/s RMS.

## Electrical Faults

**Purpose:** Broaden fault coverage.

**Configuration:**
- Single-phase operation: Disconnect one phase
- Voltage unbalance: Reduce one phase by 10%, 20%

**Test speeds:** 1000, 1500 RPM only (reduced load)

**Expected signature:** 2× line frequency sidebands

## Data Collection Protocol

**Per condition:**
1. Set speed via VFD
2. Wait 60 seconds for stabilization
3. Record 60 seconds of data
4. Save with label: `[condition]_[speed]_[trial].bin`

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