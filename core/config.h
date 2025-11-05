#ifndef CONFIG_H
#define CONFIG_H

// Auto-detect platform (ESP32 or RP2350 only)
#if defined(ARDUINO_ARCH_ESP32)
  #define PLATFORM_ESP32
  #define LED_PIN 2
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #define PLATFORM_RP2350
  #define LED_PIN LED_BUILTIN
  #define HAS_WIFI
#else
  #error "Unsupported platform. Use ESP32-S3 or RP2350 (Pico 2 W)"
#endif

// WiFi Configuration
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883

// Model Configuration
#define NUM_CLUSTERS 3
#define FEATURE_DIM 2
#define LEARNING_RATE 0.2f

#endif