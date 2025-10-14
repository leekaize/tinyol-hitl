# RP2350 Platform Layer

Hardware wrapper for Raspberry Pi Pico 2 W/WH.

## Hardware

**Board:** Raspberry Pi Pico 2 W (RP2350B + CYW43439 WiFi)  
**Clock:** 150 MHz (default)  
**RAM:** 520 KB available  
**Flash:** 4 MB (XIP)

## Build

**Prerequisites:**
```bash
# Install Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init
export PICO_SDK_PATH=~/pico-sdk

# Install ARM toolchain
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++
```

**Compile:**
```bash
cd platforms/rp2350
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2_w -DPICO_SDK_PATH=$HOME/pico-sdk
make
```

**Flash:**
1. Hold BOOTSEL button, plug USB
2. Copy `build/kmeans_test.uf2` to mounted drive
3. Pico reboots, runs test

## Serial Output

```bash
sudo minicom -D /dev/ttyACM0 -b 115200
```

Or screen: `screen /dev/ttyACM0 115200`

## LED Indicators

**On boot:** 3 blinks (200ms interval)  
**Processing:** Rapid flicker (10ms per point)  
**Complete:** Long-short-long pattern (infinite loop)

## Test Behavior

Generates 150 synthetic points from 3 clusters:
- Cluster A: (-1, -1) ± 0.2
- Cluster B: (1, 1) ± 0.2  
- Cluster C: (0, 0) ± 0.2

Prints stats every 50 points. Converges in <1 second.

## Files

- `kmeans_platform.h` - Platform API
- `kmeans_platform.c` - RP2350 implementation
- `main.c` - Test application
- `CMakeLists.txt` - Pico SDK build config

## Performance

**Throughput:** 150 points/sec @ 150MHz  
**Memory:** 4.2 KB model + 16 KB stack  
**Power:** ~40 mA active (USB serial enabled)

## Notes

Pico 2 W LED controlled via WiFi chip (CYW43), not GPIO 25. Requires `pico_cyw43_arch_none` library link.