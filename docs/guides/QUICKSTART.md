# Quick Start

**1. Test core algorithm:**
```bash
cd core/clustering
gcc -o test_kmeans test_kmeans.c streaming_kmeans.c -lm
./test_kmeans
```

**2. Flash RP2350:**
```bash
cd platforms/rp2350/build
cmake .. -DPICO_BOARD=pico2_w -DPICO_SDK_PATH=$HOME/pico-sdk
make
# Copy kmeans_test.uf2 to Pico in BOOTSEL mode
```

**3. View output:**
```bash
minicom -D /dev/ttyACM0 -b 115200
```