# Lab Notebook

---

## Day 1 - 2025-10-12 (Sunday, 5hr)

**Goal:** Validate RP2350 toolchain and WiFi stack.

**Completed:**
- Toolbox container + ARM GCC 15.1.0 installed
- Pico SDK cloned with submodules (critical: WiFi firmware blob verified)
- Built hello_usb and wifi_scan examples
- Flashed via BOOTSEL, serial working, networks detected

**Key blocker:** WiFi examples default UART not USB stdio.  
**Fix:** Added `pico_enable_stdio_usb()` to CMakeLists.txt.

**Decisions:**
- Use native GCC 15 (works, faster than ARM official)
- System-level minicom (hardware access cleaner on host)
- Public repo Apache-2.0 (industrial-friendly, timestamps work)

**Tomorrow:** Implement online k-means (~200 lines). Unit test with synthetic data.

---

## Day 2 - 2025-10-13 (TBD)

[Template for tomorrow]