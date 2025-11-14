/**
 * @file config.h
 * @brief Platform and network configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// DOIT ESP32 DEVKIT V1 pin mapping
#if defined(ARDUINO_ESP32_DEV)
  #define PLATFORM_ESP32
  #define I2C_SDA 21
  #define I2C_SCL 22
  #define ADC_CURRENT_L1 36  // VP
  #define ADC_CURRENT_L2 39  // VN
  #define ADC_CURRENT_L3 34
  #define HAS_WIFI
#endif

// RP2350 Pico 2 W
#if defined(ARDUINO_RASPBERRY_PI_PICO_2)
  #define PLATFORM_RP2350
  #define I2C_SDA 4   // GP4
  #define I2C_SCL 5   // GP5
  #define ADC_CURRENT_L1 26  // ADC0
  #define ADC_CURRENT_L2 27  // ADC1
  #define ADC_CURRENT_L3 28  // ADC2
  #define HAS_WIFI
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