#ifndef CONFIG_H
#define CONFIG_H

// ===== OPERATION MODE =====
// Uncomment ONE mode:
#define MODE_DATASET       // Stream CWRU via Serial
// #define MODE_SENSOR     // Live sensor reading (not implemented)

// ===== PLATFORM AUTO-DETECT =====
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

// ===== MODEL LIMITS =====
#define MAX_FEATURES 64    // Must match streaming_kmeans.h

// ===== WIFI CONFIG (future use) =====
#ifdef HAS_WIFI
  #define WIFI_SSID "YourSSID"
  #define WIFI_PASSWORD "YourPassword"
  #define MQTT_BROKER "192.168.1.100"
  #define MQTT_PORT 1883
#endif

#endif