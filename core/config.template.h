/**
 * @file config.template.h
 * @brief Configuration template - COPY TO config.h AND EDIT
 *
 * SETUP:
 * 1. cp config.template.h config.h
 * 2. Edit config.h with your settings
 * 3. config.h is gitignored
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PLATFORM DETECTION (Auto-detected, don't change)
// =============================================================================

#if defined(ESP32) || defined(ARDUINO_ESP32_DEV)
  #define PLATFORM_ESP32
  #define I2C_SDA 21
  #define I2C_SCL 22
  #define ADC_CURRENT_L1 34  // GPIO34 (ADC1_CH6)
  #define ADC_CURRENT_L2 35  // GPIO35 (ADC1_CH7)
  #define ADC_CURRENT_L3 36  // GPIO36 (VP, ADC1_CH0)
  #define HAS_WIFI
#elif defined(ARDUINO_ARCH_RP2040)
  #define PLATFORM_RP2350
  #define I2C_SDA 4
  #define I2C_SCL 5
  #define ADC_CURRENT_L1 26  // ADC0
  #define ADC_CURRENT_L2 27  // ADC1
  #define ADC_CURRENT_L3 28  // ADC2
  #define HAS_WIFI
#else
  #error "Unsupported platform"
#endif

// =============================================================================
// SENSOR SELECTION (Uncomment what you have connected)
// =============================================================================

// Accelerometer: Choose ONE
// #define SENSOR_ACCEL_ADXL345
#define SENSOR_ACCEL_MPU6050

// Current sensors: Uncomment if ZMCT103C installed
// #define SENSOR_CURRENT_ZMCT103C

// =============================================================================
// WIFI CREDENTIALS (Edit these!)
// =============================================================================

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// =============================================================================
// MQTT BROKER
// =============================================================================

// Public test broker (no auth, not for production)
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""

// Local broker example:
// #define MQTT_BROKER "192.168.1.100"

// =============================================================================
// DEVICE IDENTITY
// =============================================================================

#define DEVICE_ID "tinyol_motor01"

// MQTT topics (auto-generated):
// sensor/{DEVICE_ID}/data      - Published summaries
// tinyol/{DEVICE_ID}/label     - Receive labels
// tinyol/{DEVICE_ID}/discard   - Receive discards

// =============================================================================
// ALGORITHM PARAMETERS
// =============================================================================

// Sampling rate
#define SAMPLE_RATE_HZ 10

// Outlier detection: 2.0 = triggers at 2× cluster radius
// Lower = more sensitive, Higher = fewer false alarms
#define OUTLIER_THRESHOLD 2.0f

// Learning rate: 0.1-0.3 typical
// Higher = faster adaptation, Lower = more stable
#define LEARNING_RATE 0.2f

// =============================================================================
// CURRENT SENSOR CALIBRATION
// =============================================================================

#ifdef SENSOR_CURRENT_ZMCT103C
  #define CT_BURDEN_RESISTOR 100.0f  // Ohms (on breakout board)
  #define CT_SENSITIVITY 0.1f        // V/A (adjust with known load)
  #define CT_NOISE_FLOOR 0.05f       // Below this = 0A
  #define CT_SAMPLES 2000            // Samples per RMS measurement
#endif

// =============================================================================
// DEBUG OPTIONS
// =============================================================================

// Uncomment to enable verbose output
// #define DEBUG_FEATURES
// #define DEBUG_CLUSTERING
// #define DEBUG_MQTT

#endif // CONFIG_H