# Power Profiling

Measure. Compare. Optimize. Three platforms. One year battery target.

## Why This Matters

Battery life = deployment viability. Industrial sensors run for months without charging. Architecture choice (Xtensa vs ARM vs AVR) impacts power budget. Measure to decide.

## 1-Year Battery Target

**Calculation:**
- Battery: 10,000 mAh @ 3.7V = 37 Wh
- Average current needed: 37 Wh / (365 days × 24 hours) = 4.2 mA
- Duty cycle: 5% active (sampling), 95% sleep

**Allowed current:**
- Active mode: 50 mA × 0.05 = 2.5 mA avg contribution
- Sleep mode: 1 mA × 0.95 = 0.95 mA avg contribution
- Total: 3.45 mA average (target: <4.2 mA)

**Verdict:** Achievable if sleep mode <1 mA and active <50 mA.

## Measurement Tools

### Nordic PPK2 (Recommended)

**Best tool for MCU profiling.**

**Specs:**
- Range: 1 µA to 1 A
- Resolution: 200 nA
- Sampling: 100 kHz (captures WiFi spikes)
- Price: $100
- Interface: USB, desktop software

**Why:** Captures fast transients. MQTT bursts visible.

### INA219 (Budget Option)

**$10 I²C current sensor.**

**Specs:**
- Range: 0-3.2 A
- Resolution: 0.8 mA
- Sampling: ~100 Hz (misses fast transients)
- Interface: I²C to Raspberry Pi or another Arduino

**Limitation:** Misses WiFi TX bursts (<10 ms). Use for steady-state only.

### Oscilloscope + Shunt (Lab-Grade)

**For precision measurements.**

**Setup:** 0.1Ω shunt resistor in series with VDD. Measure voltage drop.

**Math:** Current = V_shunt / 0.1Ω

**Advantage:** Captures all transients. Time-correlate with logic analyzer.

**Disadvantage:** Complex setup. Not portable.

## Methodology

### Power Path Setup
```
Battery/Bench PSU → Power Meter → MCU VDD → GND
```

**Critical:** Insert meter between power source and MCU. Don't measure via USB (voltage drops under load).

### Arduino IDE Integration

**Add debug markers to core.ino:**
```cpp
void setup() {
  pinMode(DEBUG_PIN, OUTPUT);
  // ... rest of setup
}

void loop() {
  digitalWrite(DEBUG_PIN, HIGH);  // Mark measurement region

  // Process samples
  sensors_event_t event;
  accel.getEvent(&event);
  uint8_t cluster = kmeans_update(&model, features);

  digitalWrite(DEBUG_PIN, LOW);   // End measurement region

  delay(100);  // Sampling interval
}
```

**Logic analyzer on DEBUG_PIN syncs power trace with code execution.**

### Operating Modes to Profile

**1. Idle (WiFi off)**
- No sensor reading, no processing
- Duration: 60 seconds
- Measures: ESP32 vs RP2350 vs Arduino idle current

**2. Sensor Reading + Inference**
- Read ADXL345, run k-means
- Duration: 1000 samples
- Measures: Compute current at full speed

**3. WiFi TX (ESP32/RP2350 only)**
- Send cluster ID via MQTT (QoS 0)
- Duration: 100 ms burst
- Measures: TX energy per publish

**4. Deep Sleep**
- WiFi off, CPU halted, RAM retained
- Duration: 60 seconds
- Measures: Sleep current

**5. Flash Write**
- Persist model to NVS/LittleFS/EEPROM
- Duration: ~10 ms
- Measures: Write energy

## Expected Results

### ESP32-S3

**Idle:** 5-10 mA (WiFi stack in memory)

**Inference:** 40-50 mA @ 240 MHz

**WiFi TX:** 150-200 mA burst (10-50 ms)

**Sleep:** 10 µA (ULP coprocessor on)

**Flash write:** 20 mA spike (10 ms)

### RP2350 (Pico 2 W)

**Idle:** 2-5 mA (CYW43 dormant)

**Inference:** 30-40 mA @ 150 MHz

**WiFi TX:** 120-150 mA burst (10-50 ms)

**Sleep:** 0.8 mA (RAM retention, CYW43 off)

**Flash write:** 15 mA spike (8 ms)

### Arduino Uno (No WiFi)

**Idle:** 15-20 mA (16 MHz AVR, power LED on)

**Inference:** 20-25 mA

**Sleep (power-down mode):** 15 µA

**EEPROM write:** 18 mA spike (4 ms)

**Hypothesis:** RP2350 lowest idle/sleep. ESP32 highest compute performance. Arduino lowest active for simple tasks.

## Optimization Strategies

### 1. Reduce Active Time
**Strategy:** Process faster, sleep more.

**Tactics:**
- Increase clock during inference (finish faster)
- Batch MQTT messages (one TX per 100 samples)
- Pre-compute lookup tables

**Trade:** Higher peak current, shorter duration. Net energy savings if sleep increases.

### 2. WiFi Power Management
**Strategy:** Disconnect when idle.

**Arduino IDE example:**
```cpp
#ifdef HAS_WIFI
void smartSleep() {
  WiFi.disconnect(true);  // Turn off radio
  WiFi.mode(WIFI_OFF);
  delay(60000);           // Sleep 60 seconds
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
#endif
```

**Trade:** Reconnection overhead (1-2 seconds @ 150 mA).

**Decision:** If transmit interval >10 seconds, disconnect WiFi. Otherwise keep modem sleep.

### 3. Clock Scaling
**Strategy:** Run slower when accuracy allows.

**Arduino IDE example (ESP32):**
```cpp
setCpuFrequencyMhz(80);  // Drop from 240 MHz to 80 MHz
// Do inference
setCpuFrequencyMhz(240); // Boost for WiFi
```

**Trade:** Latency vs energy. Measure energy-delay product (EDP = E × t²).

**Decision:** Minimize EDP, not just energy. Faster sometimes wins.

### 4. Flash Wear vs Power
**Strategy:** Write less frequently.

**Tactics:**
- Buffer 100 updates, write once
- Only persist on significant model change

**Trade:** Data loss risk if power fails. Acceptable for non-critical applications.

## Comparison Metrics

**Create table:**

| Metric | ESP32-S3 | RP2350 | Arduino | Winner |
|--------|----------|--------|---------|--------|
| Idle current | [X mA] | [Y mA] | [Z mA] | [Platform] |
| Inference current @ max MHz | [X mA] | [Y mA] | [Z mA] | [Platform] |
| WiFi TX burst | [X mA] | [Y mA] | N/A | [Platform] |
| Sleep current | [X µA] | [Y µA] | [Z µA] | [Platform] |
| Energy per 1000 samples | [X mJ] | [Y mJ] | [Z mJ] | [Platform] |
| Battery life (95% sleep) | [X days] | [Y days] | [Z days] | [Platform] |

**Winner criteria:** Lowest average current for target duty cycle (5% active, 95% sleep).

## Validation

**Sanity check:** Does measured current match datasheet?

**ESP32-S3 datasheet:** 40 mA active, 10 µA deep sleep

**RP2350 datasheet:** 50 mA active @ 150 MHz, 0.18 mA dormant

**Arduino Uno datasheet:** 20 mA active, 15 µA power-down

**If mismatch >20%:** Check measurement setup. Verify no external peripherals drawing current.

**Long-term test:** Run on battery for 7 days. Extrapolate to 1 year. Compare to model.

## Arduino IDE Tool Setup

### PPK2 (Recommended)
1. Install nRF Connect for Desktop
2. Launch Power Profiler app
3. Connect PPK2 between PSU and MCU
4. Set source voltage: 3.3V
5. Set measurement range: 1 mA (low noise)
6. Record for 60 seconds per test
7. Export to CSV for analysis

### INA219 (Budget)
**Python script for Raspberry Pi:**
```python
import ina219
import time

sensor = ina219.INA219(0x40)
sensor.configure()

for _ in range(600):  # 60 seconds @ 10 Hz
    current_mA = sensor.current()
    print(f"{time.time()},{current_mA}")
    time.sleep(0.1)
```

**Save output, analyze in spreadsheet.**

### Oscilloscope Method
1. Insert 0.1Ω shunt in series with VDD
2. Scope CH1: Voltage across shunt
3. Scope CH2: GPIO debug signal
4. Math: Current = V_shunt / 0.1Ω
5. Trigger on GPIO rising edge

## Analysis Script

**Python template:**
```python
# tools/power_profiling/analyze.py
import pandas as pd

df = pd.read_csv('power_trace.csv')
mean_current = df['current_mA'].mean()
energy_mAh = mean_current * (len(df) / 3600)  # Assuming 1 Hz sampling

print(f"Mean: {mean_current:.2f} mA")
print(f"Energy: {energy_mAh:.4f} mAh")
print(f"Battery life @ 10Ah: {10000 / mean_current / 24:.1f} days")
```

## Common Pitfalls

**1. USB power measurement:** Don't measure via USB. Voltage drops under load. Use battery or bench PSU.

**2. Bypass capacitors:** MCU has capacitors on PCB. They smooth spikes. You see average, not peak.

**3. WiFi certification:** Operating >100 mW EIRP needs FCC/CE certification. Check regional limits.

**4. Temperature:** Current increases with temperature. Measure at operating temp (e.g., 40°C).

## Arduino IDE Workflow for Testing

**1. Upload baseline sketch:**
```bash
Tools → Board → Select your board
Sketch → Upload
```

**2. Connect power meter:**
```
Battery → PPK2 → MCU VDD → GND
```

**3. Start recording:**
- PPK2: Click "Start"
- INA219: Run Python script
- Scope: Set trigger, press single capture

**4. Let run for 60 seconds**

**5. Export data, analyze**

**6. Modify sketch for next mode:**
```cpp
// Comment out WiFi
#undef HAS_WIFI
```

**7. Re-upload, repeat measurement**

## Next Steps (Priority Order)

1. **Order PPK2 or build INA219 circuit** - Can't optimize what you don't measure
2. **Measure ESP32 baseline** - All 5 modes
3. **Measure RP2350 baseline** - All 5 modes
4. **Create comparison table** - Who wins?
5. **Optimize worst offender** - Target highest current mode
6. **Re-measure** - Did it improve?
7. **7-day battery test** - Validate model

**Ship baseline measurements first. Optimize later.**

## Arduino Serial Monitor Logging

**Add to core.ino for real-time power debugging:**
```cpp
void logPowerState(const char* state) {
  Serial.printf("[POWER] State: %s, Uptime: %lu ms\n",
                state, millis());
}

void loop() {
  logPowerState("ACTIVE");
  // ... do work ...

  logPowerState("SLEEP");
  delay(60000);
}
```

**Sync Serial output with power trace timestamps for correlation.**

**One measurement setup. Three platforms. Zero guesswork.**