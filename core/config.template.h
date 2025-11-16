/**
 * @file config.template.h
 * @brief Configuration template
 *
 * SETUP:
 * 1. Copy this file to config.h
 * 2. Edit config.h with your settings
 * 3. config.h is gitignored - never commit it
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===== PLATFORM DETECTION =====
#if defined(ESP32) || defined(ARDUINO_ESP32_DEV)
  #define PLATFORM_ESP32
  #define I2C_SDA 21
  #define I2C_SCL 22
  #define ADC_CURRENT_L1 36  // VP
  #define ADC_CURRENT_L2 39  // VN
  #define ADC_CURRENT_L3 34
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040)
  #define PLATFORM_RP2350
  #define I2C_SDA 4   // GP4
  #define I2C_SCL 5   // GP5
  #define ADC_CURRENT_L1 26  // ADC0
  #define ADC_CURRENT_L2 27  // ADC1
  #define ADC_CURRENT_L3 28  // ADC2
  #define HAS_WIFI
#else
  #error "Unsupported platform. Edit config.h to add your board."
#endif

// ===== SENSOR SELECTION =====
// Uncomment ONE accelerometer (what you have connected):
// #define SENSOR_ACCEL_ADXL345
#define SENSOR_ACCEL_MPU6050  // GY521

// Uncomment if current sensors installed:
// #define SENSOR_CURRENT_ZMCT103C

// ===== FEATURE EXTRACTION =====
// Time-domain features from vibration
#define FEATURE_WINDOW_SIZE 25    // Samples to buffer (2.5s at 10Hz)
#define FEATURE_SCHEMA_VERSION 1  // Increment when adding features

// Current schema v1: [rms, peak, crest] from accelerometer
// Future v2: Add [kurtosis]
// Future v3: Add [i1, i2, i3] when current sensors connected

// ===== WIFI CREDENTIALS =====
// TODO: Replace with your WiFi network
#define WIFI_SSID "YOUR_WIFI_SSID_HERE"
#define WIFI_PASS "YOUR_WIFI_PASSWORD_HERE"

// ===== MQTT BROKER =====
// Default: Public HiveMQ broker (testing only)
// Production: Replace with your MQTT server
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER ""      // Leave empty for anonymous
#define MQTT_PASS ""

// ===== DEVICE IDENTITY =====
// TODO: Change for each device you deploy
#define DEVICE_ID "tinyol_device1"

// MQTT topics (auto-generated from DEVICE_ID):
// Publishes to: sensor/{DEVICE_ID}/data
// Subscribes to: tinyol/{DEVICE_ID}/label

// ===== SAMPLING RATE =====
#define SAMPLE_RATE_HZ 10  // 10 Hz = 100ms interval

// ===== CT SENSOR PARAMETERS =====
// Only used if SENSOR_CURRENT_ZMCT103C defined
#define CT_BURDEN_RESISTOR 100.0f  // Ohms
#define CT_TURNS_RATIO 1000.0f     // 1000:1

#ifndef ADC_RESOLUTION
#define ADC_RESOLUTION 4096.0f   // 12-bit (RP2040 already defines this)

#endif

#define ADC_VREF 3.3f              // Reference voltage

#endif // CONFIG_H