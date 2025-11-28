/**
 * @file core.ino
 * @brief TinyOL-HITL: Label-driven clustering with vibration + current
 * 
 * Features:
 * - 7D feature vector [vib_rms, vib_peak, vib_crest, i1, i2, i3, i_rms]
 * - Freeze-on-alarm workflow
 * - MQTT integration for SCADA
 * - Cross-platform (ESP32/RP2350)
 */

#include "config.h"
#include "streaming_kmeans.h"
#include "feature_extractor.h"
#include <Wire.h>

#ifndef LED_BUILTIN
  #if defined(ESP32)
    #define LED_BUILTIN 2
  #elif defined(ARDUINO_ARCH_RP2040)
    #define LED_BUILTIN 25
  #else
    #define LED_BUILTIN 13
  #endif
#endif

// =============================================================================
// Sensor Includes
// =============================================================================

#ifdef SENSOR_ACCEL_MPU6050
  #include <Adafruit_MPU6050.h>
  #include <Adafruit_Sensor.h>
  Adafruit_MPU6050 mpu;
#endif

#ifdef SENSOR_ACCEL_ADXL345
  #include <Adafruit_ADXL345_U.h>
  Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
#endif

#ifdef HAS_WIFI
  #include <PubSubClient.h>
  #include <ArduinoJson.h>
  #ifdef ESP32
    #include <WiFi.h>
  #else
    #include <WiFi.h>
  #endif
#endif

// =============================================================================
// Globals
// =============================================================================

kmeans_model_t model;

#ifdef SENSOR_CURRENT_ZMCT103C
  CurrentSensor currentSensor;
#endif

#ifdef HAS_WIFI
  WiFiClient wifiClient;
  PubSubClient mqtt(wifiClient);
  char mqtt_topic_pub[64];
  char mqtt_topic_label[64];
  char mqtt_topic_discard[64];
#endif

// Windowing
const int WINDOW_SIZE = 100;
float window_features[WINDOW_SIZE][FEATURE_DIM];
int window_idx = 0;
unsigned long last_sample = 0;
unsigned long last_publish = 0;
const int SAMPLE_MS = 100;    // 10 Hz
const int PUBLISH_MS = 10000; // 10 seconds

// =============================================================================
// Platform Init
// =============================================================================

void platform_init() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== TinyOL-HITL v2 ===");
  #ifdef ESP32
    Serial.println("Platform: ESP32");
  #elif defined(ARDUINO_ARCH_RP2040)
    Serial.println("Platform: RP2350");
  #endif
  Serial.printf("Features: %dD\n", FEATURE_DIM);
  
  pinMode(LED_BUILTIN, OUTPUT);

  // I2C
  #ifdef ARDUINO_ARCH_RP2040
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif

  // ADC for current sensors
  #ifdef SENSOR_CURRENT_ZMCT103C
    #ifdef ESP32
      analogReadResolution(12);
      analogSetAttenuation(ADC_11db);
    #endif
  #endif

  // WiFi
  #ifdef HAS_WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf(" OK: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println(" FAIL");
    }
  #endif
}

// =============================================================================
// MQTT
// =============================================================================

#ifdef HAS_WIFI
void mqtt_callback(char* topic, byte* payload, unsigned int len) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) {
    Serial.println("JSON parse error");
    return;
  }

  if (strstr(topic, "/label")) {
    const char* label = doc["label"];
    if (label && kmeans_add_cluster(&model, label)) {
      Serial.printf("✓ Labeled: %s (K=%d)\n", label, model.k);
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH); delay(100);
        digitalWrite(LED_BUILTIN, LOW); delay(100);
      }
    } else {
      Serial.println("✗ Label failed");
    }
  }

  if (strstr(topic, "/discard")) {
    if (doc["discard"] == true) {
      kmeans_discard(&model);
      Serial.println("✓ Discarded");
    }
  }
}

bool mqtt_connect() {
  if (!WiFi.isConnected()) return false;
  
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);
  
  bool ok = strlen(MQTT_USER) > 0 
    ? mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)
    : mqtt.connect(DEVICE_ID);
  
  if (ok) {
    snprintf(mqtt_topic_pub, sizeof(mqtt_topic_pub), "sensor/%s/data", DEVICE_ID);
    snprintf(mqtt_topic_label, sizeof(mqtt_topic_label), "tinyol/%s/label", DEVICE_ID);
    snprintf(mqtt_topic_discard, sizeof(mqtt_topic_discard), "tinyol/%s/discard", DEVICE_ID);
    
    mqtt.subscribe(mqtt_topic_label);
    mqtt.subscribe(mqtt_topic_discard);
    Serial.println("✓ MQTT connected");
    return true;
  }
  
  Serial.printf("✗ MQTT failed: %d\n", mqtt.state());
  return false;
}

void publish_summary(bool is_alarm) {
  if (!mqtt.connected()) return;

  // Compute window statistics
  float vib_rms_sum = 0, vib_rms_max = 0;
  float vib_peak_sum = 0, vib_peak_max = 0;
  float vib_crest_sum = 0, vib_crest_max = 0;
  #ifdef SENSOR_CURRENT_ZMCT103C
  float i_rms_sum = 0, i_rms_max = 0;
  #endif

  int n = (window_idx > 0) ? window_idx : WINDOW_SIZE;
  for (int i = 0; i < n; i++) {
    vib_rms_sum += window_features[i][0];
    vib_peak_sum += window_features[i][1];
    vib_crest_sum += window_features[i][2];
    if (window_features[i][0] > vib_rms_max) vib_rms_max = window_features[i][0];
    if (window_features[i][1] > vib_peak_max) vib_peak_max = window_features[i][1];
    if (window_features[i][2] > vib_crest_max) vib_crest_max = window_features[i][2];
    #ifdef SENSOR_CURRENT_ZMCT103C
    i_rms_sum += window_features[i][6];
    if (window_features[i][6] > i_rms_max) i_rms_max = window_features[i][6];
    #endif
  }

  system_state_t state = kmeans_get_state(&model);
  
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["cluster"] = is_alarm ? -1 : 0;
  doc["label"] = is_alarm ? "unknown" : "normal";
  doc["k"] = model.k;
  doc["alarm_active"] = (state == STATE_FROZEN || state == STATE_FROZEN_IDLE);
  doc["frozen"] = (state == STATE_FROZEN || state == STATE_FROZEN_IDLE);
  doc["idle"] = kmeans_is_idle(&model);
  doc["sample_count"] = n;
  doc["vib_rms_avg"] = vib_rms_sum / n;
  doc["vib_rms_max"] = vib_rms_max;
  doc["vib_peak_avg"] = vib_peak_sum / n;
  doc["vib_peak_max"] = vib_peak_max;
  doc["vib_crest_avg"] = vib_crest_sum / n;
  doc["vib_crest_max"] = vib_crest_max;
  #ifdef SENSOR_CURRENT_ZMCT103C
  doc["current_rms_avg"] = i_rms_sum / n;
  doc["current_rms_max"] = i_rms_max;
  #endif
  doc["buffer_samples"] = kmeans_get_buffer_size(&model);
  doc["timestamp"] = millis();

  char buf[512];
  serializeJson(doc, buf);
  mqtt.publish(mqtt_topic_pub, buf);
  
  if (is_alarm) Serial.println("⚠️ ALARM PUBLISHED");
}
#endif

// =============================================================================
// Setup
// =============================================================================

void setup() {
  platform_init();

  // Init accelerometer
  #ifdef SENSOR_ACCEL_MPU6050
    if (!mpu.begin()) {
      Serial.println("✗ MPU6050 not found");
      while (1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("✓ MPU6050");
  #endif

  #ifdef SENSOR_ACCEL_ADXL345
    if (!accel.begin()) {
      Serial.println("✗ ADXL345 not found");
      while (1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); }
    }
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("✓ ADXL345");
  #endif

  // Init current sensors
  #ifdef SENSOR_CURRENT_ZMCT103C
    currentSensor.calibrate();
    Serial.println("✓ ZMCT103C");
  #endif

  // Init clustering model
  if (!kmeans_init(&model, FEATURE_DIM, 0.2f)) {
    Serial.println("✗ Model init failed");
    while (1) delay(1000);
  }
  Serial.printf("✓ Model: K=%d, dim=%d, size=%d bytes\n", 
                model.k, FEATURE_DIM, sizeof(kmeans_model_t));

  #ifdef HAS_WIFI
    if (WiFi.isConnected()) mqtt_connect();
  #endif

  Serial.println("=== Ready ===\n");
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
  unsigned long now = millis();

  #ifdef HAS_WIFI
    mqtt.loop();
  #endif

  // Sample at 10 Hz
  if (now - last_sample < SAMPLE_MS) return;
  last_sample = now;

  // Skip if frozen
  system_state_t state = kmeans_get_state(&model);
  if (state == STATE_FROZEN) return;

  // Read accelerometer
  float ax, ay, az;
  #ifdef SENSOR_ACCEL_MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
  #endif
  #ifdef SENSOR_ACCEL_ADXL345
    sensors_event_t event;
    accel.getEvent(&event);
    ax = event.acceleration.x;
    ay = event.acceleration.y;
    az = event.acceleration.z;
  #endif

  // Read current sensors
  float i1 = 0, i2 = 0, i3 = 0;
  #ifdef SENSOR_CURRENT_ZMCT103C
    currentSensor.read(&i1, &i2, &i3);
  #endif

  // Extract features
  float features_f[FEATURE_DIM];
  #ifdef SENSOR_CURRENT_ZMCT103C
    FeatureExtractor::extractCombined(ax, ay, az, i1, i2, i3, features_f);
  #else
    FeatureExtractor::extractVibration(ax, ay, az, features_f);
  #endif

  // Store in window
  for (int i = 0; i < FEATURE_DIM; i++) {
    window_features[window_idx][i] = features_f[i];
  }
  window_idx = (window_idx + 1) % WINDOW_SIZE;

  // Update idle detection
  fixed_t rms_fixed = FLOAT_TO_FIXED(features_f[0]);
  #ifdef SENSOR_CURRENT_ZMCT103C
    fixed_t current_fixed = FLOAT_TO_FIXED(features_f[6]);
  #else
    fixed_t current_fixed = 0;
  #endif
  kmeans_update_idle(&model, rms_fixed, current_fixed);

  // Convert to fixed-point
  fixed_t features[FEATURE_DIM];
  for (int i = 0; i < FEATURE_DIM; i++) {
    features[i] = FLOAT_TO_FIXED(features_f[i]);
  }

  // Update clustering
  int8_t cluster_id = kmeans_update(&model, features);

  // Check alarm
  if (cluster_id == -1 && kmeans_get_state(&model) == STATE_FROZEN) {
    Serial.println("⚠️ OUTLIER - FROZEN");
    #ifdef HAS_WIFI
      if (mqtt.connected()) publish_summary(true);
    #endif
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, HIGH); delay(100);
      digitalWrite(LED_BUILTIN, LOW); delay(100);
    }
    return;
  }

  // Publish summary every 10s
  if (now - last_publish >= PUBLISH_MS) {
    last_publish = now;

    char label[MAX_LABEL_LENGTH];
    if (cluster_id >= 0) kmeans_get_label(&model, cluster_id, label);
    else strcpy(label, "normal");

    Serial.printf("C%d(%s) RMS:%.2f", cluster_id, label, features_f[0]);
    #ifdef SENSOR_CURRENT_ZMCT103C
      Serial.printf(" I:%.2f/%.2f/%.2f", i1, i2, i3);
    #endif
    Serial.printf(" K=%d\n", model.k);

    #ifdef HAS_WIFI
      if (mqtt.connected()) publish_summary(false);
      else mqtt_connect();
    #endif

    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
  }
}