/**
 * @file config.h
 * @brief Platform and network configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// Platform detection
#if defined(ESP32)
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040)
  #define HAS_WIFI
#else
  #warning "Unknown platform - WiFi disabled"
#endif

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"

// MQTT settings
#define MQTT_BROKER "broker.hivemq.com"  // Public broker for testing
#define MQTT_PORT 1883
#define DEVICE_ID "tinyol_device1"       // Change for multiple devices

// Sensor configuration
#define SENSOR_TYPE_ADXL345
// #define SENSOR_TYPE_MPU6050  // Alternative (not implemented)

// Sampling rate
#define SAMPLE_RATE_HZ 10  // 10 Hz (100ms interval)

#endif // CONFIG_H