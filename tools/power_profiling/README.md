# Power Profiling

Compare ESP32, RP2350, Arduino current consumption.

## Tools
**Nordic PPK2:** $100, 1µA-1A, 100kHz sampling
**INA219:** $10, 0-3.2A, 100Hz sampling

## Modes
1. Idle (WiFi off, 60s)
2. Inference (process 1000 samples)
3. WiFi TX (MQTT burst) - ESP32/RP2350 only
4. Sleep (deep sleep, 60s)
5. Flash write (persist model)

## Targets
**ESP32:** 45 mA active, 10 µA sleep
**RP2350:** 35 mA active, 0.8 mA sleep
**Arduino:** 20 mA active (no WiFi), 15 µA sleep (power-down)

1-year battery: <4.2 mA average (10Ah @ 3.7V).

See docs/POWER.md for methodology.