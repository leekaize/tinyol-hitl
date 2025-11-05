# Power Profiling

Measure. Compare. Optimize. ESP32 vs RP2350 power consumption across operating modes.

## Why This Matters

Battery life determines deployment viability. 1-year target requires <1% active duty cycle.

Architecture choice (Xtensa vs ARM) impacts power budget. Measure to decide.

## Target Budget

**1-year battery life calculation:**
- Battery: 10,000 mAh @ 3.7V = 37 Wh
- Average current: 37 Wh / (365 days × 24 hours) = 4.2 mA
- Duty cycle: 5% active (WiFi on), 95% sleep (WiFi off)

**Allowed current:**
- Active mode: 50 mA × 0.05 = 2.5 mA average contribution
- Sleep mode: 1 mA × 0.95 = 0.95 mA average contribution
- Total: 3.45 mA average (target: <4.2 mA)

**Verdict:** Achievable if sleep mode <1 mA and active <50 mA.

## Measurement Tools

### Nordic PPK2

**Best tool.** Purpose-built for MCU power profiling.

**Range:** 1 µA to 1 A

**Resolution:** 200 nA

**Sampling:** 100 kHz (captures WiFi spikes)

**Price:** $100

**Interface:** USB, desktop software

### INA219

**Budget option.** I²C current sensor.

**Range:** 0-3.2 A

**Resolution:** 0.8 mA

**Sampling:** ~100 Hz (misses fast transients)

**Price:** $10

**Interface:** I²C to another MCU or Raspberry Pi

**Limitation:** Misses WiFi TX bursts (<10 ms). Use for steady-state only.

### Oscilloscope + Shunt

**Lab-grade accuracy.** Requires bench equipment.

**Setup:** 0.1Ω shunt resistor in series with VDD. Measure voltage drop.

**Resolution:** Limited by scope (1 mV = 10 mA for 0.1Ω)

**Advantage:** Captures all transients. Time-correlate with logic analyzer.

**Disadvantage:** Complex setup. Not portable.

## Methodology

### Setup

**Power path:**
```
Battery/PSU → Power Meter → MCU → GND
```

Insert meter between power source and MCU VDD pin. Measure current continuously.

**Software instrumentation:**
```c
// Mark measurement regions with GPIO toggle
gpio_set(DEBUG_PIN, 1);   // Start region
perform_inference();
gpio_set(DEBUG_PIN, 0);   // End region
```

Logic analyzer on DEBUG_PIN synchronizes power trace with code execution.

### Operating Modes

**Baseline (idle):**
- WiFi off, no processing
- Measure: ESP32 vs RP2350 idle current
- Duration: 60 seconds

**WiFi connection:**
- Connect to AP, establish MQTT
- Measure: connection burst energy
- Duration: 5 seconds

**Inference (no WiFi):**
- Stream CWRU dataset, run k-means
- Measure: compute current at 150/240 MHz
- Duration: 1000 samples

**WiFi transmit:**
- Send cluster ID via MQTT (QoS 0)
- Measure: TX burst energy
- Duration: 100 ms

**Deep sleep:**
- WiFi off, CPU halted, RAM retained
- Measure: sleep current
- Duration: 60 seconds

**Flash write:**
- Persist model to flash
- Measure: write energy
- Duration: ~10 ms

### Data Collection

**Record per mode:**
- Mean current (mA)
- Peak current (mA)
- Energy (mAh or µWh)
- Duration (ms)

**Calculate:**
- Average power: P_avg = V × I_mean
- Energy per operation: E = P_avg × duration
- Duty cycle: % time in each mode

## Expected Results

### ESP32-S3

**Idle:** 5-10 mA (WiFi stack in memory)

**Inference:** 40-50 mA @ 240 MHz

**WiFi TX:** 150-200 mA burst (10-50 ms)

**Sleep:** 10 µA (ULP coprocessor on)

**Flash write:** 20 mA spike (10 ms)

### RP2350

**Idle:** 2-5 mA (CYW43 dormant)

**Inference:** 30-40 mA @ 150 MHz

**WiFi TX:** 120-150 mA burst (10-50 ms)

**Sleep:** 0.8 mA (RAM retention, CYW43 off)

**Flash write:** 15 mA spike (8 ms)

**Hypothesis:** RP2350 lower idle/sleep. ESP32 higher compute performance. Measure to verify.

## Optimization Strategies

### Reduce Active Time

**Strategy:** Process faster, sleep more.

**Tactics:**
- Increase clock speed during inference (finish faster)
- Batch MQTT messages (one TX per 100 samples, not per sample)
- Pre-compute lookup tables (avoid runtime calculation)

**Tradeoff:** Higher peak current, but shorter duration. Net energy savings if sleep time increases.

### WiFi Power Management

**Strategy:** Disconnect when idle.

**Tactics:**
- Modem sleep: WiFi off between transmissions (saves 100 mA)
- Light sleep: CPU halted, wake on timer (saves 30 mA)
- Deep sleep: Full power-down, wake on interrupt (saves 40 mA)

**Tradeoff:** Reconnection overhead. Measure connection energy (1-2 seconds @ 150 mA).

**Decision:** If transmit interval >10 seconds, disconnect WiFi. Otherwise keep modem sleep.

### Clock Scaling

**Strategy:** Run slower when accuracy allows.

**Tactics:**
- Inference at 80 MHz (not 240 MHz) if latency acceptable
- Dynamic frequency scaling (DFS): boost for WiFi, drop for compute

**Tradeoff:** Latency vs energy. Measure energy-delay product (EDP = E × t²).

**Decision:** Minimize EDP, not just energy. Faster sometimes wins.

### Flash Wear vs Power

**Strategy:** Write less frequently.

**Tactics:**
- Buffer 100 updates, write once (vs write every update)
- Only persist on significant model change (centroid moved >threshold)

**Tradeoff:** Data loss risk if power fails. Acceptable for non-critical applications.

## Comparison Metrics

**Create table:**

| Metric | ESP32-S3 | RP2350 | Winner |
|--------|----------|--------|--------|
| Idle current | [X mA] | [Y mA] | [Platform] |
| Inference current @ max MHz | [X mA] | [Y mA] | [Platform] |
| WiFi TX burst | [X mA] | [Y mA] | [Platform] |
| Sleep current | [X µA] | [Y µA] | [Platform] |
| Energy per 1000 samples | [X mJ] | [Y mJ] | [Platform] |
| Battery life (95% sleep) | [X days] | [Y days] | [Platform] |

**Winner criteria:** Lowest average current for target duty cycle (5% active, 95% sleep).

## Validation

**Sanity check:** Does measured current match datasheet?

**ESP32-S3 datasheet:** 40 mA active, 10 µA deep sleep

**RP2350 datasheet:** 50 mA active @ 150 MHz, 0.18 mA dormant

**If mismatch >20%:** Check measurement setup. Verify no external peripherals drawing current.

**Long-term test:** Run on battery for 7 days. Extrapolate to 1 year. Compare to model.

## Tools Setup

### PPK2 (Recommended)

```bash
# Install nRF Connect for Desktop
# Launch Power Profiler app
# Connect PPK2 between PSU and MCU
# Set source voltage: 3.3V
# Set measurement range: 1 mA or 10 mA (low noise)
# Record for 60 seconds per test
# Export to CSV for analysis
```

### INA219 (Budget)

```python
# Raspberry Pi + INA219 sensor
import ina219
import time

sensor = ina219.INA219(0x40)
sensor.configure()

for _ in range(600):  # 60 seconds @ 10 Hz
    current_mA = sensor.current()
    print(f"{time.time()},{current_mA}")
    time.sleep(0.1)
```

### Oscilloscope

```
# 0.1Ω shunt in series with VDD
# Scope CH1: Voltage across shunt
# Scope CH2: GPIO debug signal
# Math: Current = V_shunt / 0.1Ω
# Trigger on GPIO rising edge (start of measurement region)
```

## Analysis Script

```python
# tools/power_profiling/analyze.py
import pandas as pd

df = pd.read_csv('power_trace.csv')
mean_current = df['current_mA'].mean()
energy_mAh = mean_current * (len(df) / 3600)  # Assuming 1 Hz sampling
print(f"Mean: {mean_current:.2f} mA")
print(f"Energy: {energy_mAh:.4f} mAh")
```

## Pitfall Avoidance

**USB power:** Don't measure via USB. Voltage drops under load. Use battery or bench PSU.

**Bypass capacitors:** MCU has capacitors on PCB. They smooth current spikes. You see average, not peak.

**WiFi certification:** If operating >100 mW EIRP, need FCC/CE certification. Check regional limits.

**Temperature:** Current increases with temperature. Measure at operating temp (e.g., 40°C in enclosure).

## Next Steps

1. Order PPK2 or build INA219 circuit
2. Measure ESP32 baseline (all modes)
3. Measure RP2350 baseline (all modes)
4. Create comparison table
5. Optimize worst offender
6. Re-measure, iterate
7. Validate with 7-day battery test