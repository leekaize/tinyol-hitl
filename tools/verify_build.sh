#!/bin/bash
# Verify Arduino sketch compiles for all platforms

set -e

SKETCH="core/core.ino"

echo "=== Arduino Build Verification ==="
echo

echo "1. ESP32-S3"
arduino-cli compile --fqbn esp32:esp32:esp32s3 $SKETCH
echo "   ✓ ESP32 OK"
echo

echo "2. RP2350 (Pico 2 W)"
arduino-cli compile --fqbn rp2040:rp2040:rpipico2w $SKETCH
echo "   ✓ RP2350 OK"
echo

echo "3. Arduino Uno"
arduino-cli compile --fqbn arduino:avr:uno $SKETCH
echo "   ✓ Arduino OK (may warn about memory)"
echo

echo "=== All Platforms Verified ==="