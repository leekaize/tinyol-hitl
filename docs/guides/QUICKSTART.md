# Quick Start

**1. Install Arduino IDE**
```bash
# Download: https://www.arduino.cc/en/software
# Or: sudo apt install arduino  # Linux
```

**2. Add Board Support**
```
File → Preferences → Additional Board Manager URLs:
https://espressif.github.io/arduino-esp32/package_esp32_index.json,https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

Tools → Board → Boards Manager:
- Search "ESP32" → Install
- Search "Pico" → Install
```

**3. Install Libraries**
```
Tools → Manage Libraries → Search "PubSubClient" → Install
```

**4. Upload Sketch**
```bash
File → Open → core/core.ino
Tools → Board → Select your board
Tools → Port → Select USB port
Sketch → Upload
```

**5. Monitor Output**
```
Tools → Serial Monitor (115200 baud)
See: Platform detected, WiFi connected, Model initialized
```

**Done.** Under 10 minutes.