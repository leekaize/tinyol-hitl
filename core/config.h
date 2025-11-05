#ifndef CONFIG_H
#define CONFIG_H

// Auto-detect platform
#if defined(ARDUINO_ARCH_ESP32)
  #define PLATFORM_ESP32
  #define LED_PIN 2
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #define PLATFORM_RP2350
  #define LED_PIN LED_BUILTIN
  #define HAS_WIFI
#elif defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_AVR_MEGA2560)
  #define PLATFORM_ARDUINO
  #define LED_PIN 13
  // WiFi requires external shield (WiFi101, ESP8266)
#else
  #define PLATFORM_GENERIC
  #define LED_PIN LED_BUILTIN
#endif

// WiFi Configuration (if available)
#ifdef HAS_WIFI
  #define WIFI_SSID "YourSSID"
  #define WIFI_PASSWORD "YourPassword"
  #define MQTT_BROKER "192.168.1.100"
  #define MQTT_PORT 1883
#endif

// Model Configuration
#define NUM_CLUSTERS 3
#define FEATURE_DIM 2
#define LEARNING_RATE 0.2f

#endif