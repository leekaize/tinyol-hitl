/**
 * @file config.template.h
 * @brief Configuration template - COPY TO config.h AND EDIT
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PLATFORM DETECTION (Auto)
// =============================================================================

#if defined(ESP32) || defined(ARDUINO_ESP32_DEV)
  #define PLATFORM_ESP32
  #define I2C_SDA 21
  #define I2C_SCL 22
  #define ADC_CURRENT_L1 34
  #define ADC_CURRENT_L2 35
  #define ADC_CURRENT_L3 36
  #define BUTTON_PIN 0  // BOOT button on most ESP32 boards
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040)
  #define PLATFORM_RP2350
  #define I2C_SDA 4
  #define I2C_SCL 5
  #define ADC_CURRENT_L1 26
  #define ADC_CURRENT_L2 27
  #define ADC_CURRENT_L3 28
  #define BUTTON_PIN 15
  #define HAS_WIFI
#else
  #error "Unsupported platform"
#endif

// =============================================================================
// FEATURE SCHEMA SELECTION (Choose ONE)
// =============================================================================

// Schema 1: Vibration only (3D) - Minimal, proven
#define FEATURE_SCHEMA_TIME_ONLY

// Schema 2: Vibration + Current (7D) - Motor diagnosis
// #define FEATURE_SCHEMA_TIME_CURRENT

// Schema 3: Vibration + FFT (6D) - Better fault signatures
// #define FEATURE_SCHEMA_FFT_ONLY

// Schema 4: All features (10D) - Full analysis
// #define FEATURE_SCHEMA_FFT_CURRENT

// =============================================================================
// SENSOR SELECTION
// =============================================================================

// Accelerometer: Choose ONE
#define SENSOR_ACCEL_MPU6050
// #define SENSOR_ACCEL_ADXL345

// =============================================================================
// WIFI CREDENTIALS
// =============================================================================

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// =============================================================================
// MQTT BROKER
// =============================================================================

#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""

// =============================================================================
// DEVICE IDENTITY
// =============================================================================

#define DEVICE_ID "tinyol_motor01"

// =============================================================================
// ALGORITHM PARAMETERS
// =============================================================================

#define SAMPLE_RATE_HZ 10
#define OUTLIER_THRESHOLD 2.0f
#define LEARNING_RATE 0.2f

// =============================================================================
// CURRENT SENSOR CALIBRATION (if using FEATURE_SCHEMA_*_CURRENT)
// =============================================================================

#define CT_BURDEN_RESISTOR 100.0f
#define CT_SENSITIVITY 0.1f
#define CT_NOISE_FLOOR 0.05f
#define CT_SAMPLES 2000

// =============================================================================
// DEBUG
// =============================================================================

// #define DEBUG_FEATURES
// #define DEBUG_CLUSTERING
// #define DEBUG_MQTT

#endif