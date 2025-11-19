/**
 * @file core.ino
 * @brief TinyOL-HITL with freeze-on-alarm workflow
 *
 * State machine:
 * NORMAL → (outlier) → FROZEN → (label/discard) → NORMAL
 *
 * Publishes summary every 10s (not 100ms streaming)
 * Flat JSON schema for SCADA compatibility
 */

#include "config.h"
#include "streaming_kmeans.h"
#include "feature_extractor.h"
#include <Wire.h>

#ifndef LED_BUILTIN
  #if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    #define LED_BUILTIN 2
  #elif defined(ARDUINO_ARCH_RP2040)
    #define LED_BUILTIN 25
  #else
    #define LED_BUILTIN 13
  #endif
#endif

// Sensor includes
#ifdef SENSOR_ACCEL_ADXL345
  #include <Adafruit_ADXL345_U.h>
  Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
#endif

#ifdef SENSOR_ACCEL_MPU6050
  #include <Adafruit_MPU6050.h>
  #include <Adafruit_Sensor.h>
  Adafruit_MPU6050 mpu;
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

// Globals
kmeans_model_t model;

#ifdef HAS_WIFI
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
char mqtt_topic_publish[64];
char mqtt_topic_label[64];
char mqtt_topic_discard[64];
#endif

// Windowing for 10-second summaries
const int WINDOW_SIZE = 100;  // 100 samples @ 10 Hz = 10 seconds
float window_samples[WINDOW_SIZE][3];  // [rms, peak, crest]
int window_index = 0;
unsigned long last_sample = 0;
unsigned long last_publish = 0;
const int SAMPLE_INTERVAL_MS = 100;  // 10 Hz
const int PUBLISH_INTERVAL_MS = 10000;  // 10 seconds

const uint8_t FEATURE_DIM = 3;

void platform_init() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TinyOL-HITL Starting ===");

  #ifdef ESP32
    Serial.println("Platform: ESP32");
  #elif defined(ARDUINO_ARCH_RP2040)
    Serial.println("Platform: RP2350");
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  // I2C init
  #ifdef ARDUINO_ARCH_RP2040
    Wire.begin();
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
  #endif

  #ifdef HAS_WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(" Connected: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println(" Failed");
    }
  #endif
}

#ifdef HAS_WIFI
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  // Label command
  if (strstr(topic, "/label")) {
    const char* label = doc["label"];
    if (!label) {
      Serial.println("✗ Label missing");
      return;
    }

    if (kmeans_add_cluster(&model, label)) {
      Serial.print("✓ Labeled: ");
      Serial.print(label);
      Serial.print(" (K=");
      Serial.print(model.k);
      Serial.println(")");
      
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("✗ Label failed (not frozen or duplicate)");
    }
  }

  // Discard command
  if (strstr(topic, "/discard")) {
    if (doc["discard"] == true) {
      kmeans_discard(&model);
      Serial.println("✓ Alarm discarded, resuming");
    }
  }
}

bool mqtt_connect() {
  if (!WiFi.isConnected()) return false;

  Serial.print("MQTT connecting: ");
  Serial.println(MQTT_BROKER);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);

  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS);
  } else {
    connected = mqtt.connect(DEVICE_ID);
  }

  if (connected) {
    Serial.println("✓ MQTT connected");

    snprintf(mqtt_topic_label, sizeof(mqtt_topic_label),
             "tinyol/%s/label", DEVICE_ID);
    snprintf(mqtt_topic_discard, sizeof(mqtt_topic_discard),
             "tinyol/%s/discard", DEVICE_ID);
    
    mqtt.subscribe(mqtt_topic_label);
    mqtt.subscribe(mqtt_topic_discard);

    Serial.print("Subscribed: ");
    Serial.println(mqtt_topic_label);

    return true;
  }

  Serial.print("✗ MQTT failed, rc=");
  Serial.println(mqtt.state());
  return false;
}

void publish_summary(bool is_alarm) {
  if (!mqtt.connected()) return;

  // Compute statistics from window
  float rms_sum = 0, rms_max = 0;
  float peak_sum = 0, peak_max = 0;
  float crest_sum = 0, crest_max = 0;

  int valid_samples = (window_index > 0) ? window_index : WINDOW_SIZE;
  for (int i = 0; i < valid_samples; i++) {
    rms_sum += window_samples[i][0];
    peak_sum += window_samples[i][1];
    crest_sum += window_samples[i][2];

    if (window_samples[i][0] > rms_max) rms_max = window_samples[i][0];
    if (window_samples[i][1] > peak_max) peak_max = window_samples[i][1];
    if (window_samples[i][2] > crest_max) crest_max = window_samples[i][2];
  }

  float rms_avg = rms_sum / valid_samples;
  float peak_avg = peak_sum / valid_samples;
  float crest_avg = crest_sum / valid_samples;

  // Get cluster info
  int8_t cluster = is_alarm ? -1 : 0;  // -1 = unknown during alarm
  char label[MAX_LABEL_LENGTH] = "unknown";
  if (!is_alarm && model.k > 0) {
    cluster = 0;  // Will be set by last update
    kmeans_get_label(&model, cluster, label);
  }

  system_state_t state = kmeans_get_state(&model);
  uint16_t buffer_size = kmeans_get_buffer_size(&model);

  // Flat JSON schema
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["cluster"] = cluster;
  doc["label"] = label;
  doc["k"] = model.k;
  doc["alarm_active"] = (state == STATE_FROZEN);
  doc["frozen"] = (state == STATE_FROZEN);
  doc["sample_count"] = valid_samples;
  doc["rms_avg"] = rms_avg;
  doc["rms_max"] = rms_max;
  doc["peak_avg"] = peak_avg;
  doc["peak_max"] = peak_max;
  doc["crest_avg"] = crest_avg;
  doc["crest_max"] = crest_max;
  doc["buffer_samples"] = buffer_size;
  doc["timestamp"] = millis();

  char buffer[512];
  serializeJson(doc, buffer);

  snprintf(mqtt_topic_publish, sizeof(mqtt_topic_publish),
           "sensor/%s/data", DEVICE_ID);
  mqtt.publish(mqtt_topic_publish, buffer);

  if (is_alarm) {
    Serial.println("⚠️  ALARM PUBLISHED");
  }
}
#endif

void setup() {
  platform_init();

  // Init accelerometer
  #ifdef SENSOR_ACCEL_ADXL345
    if (!accel.begin()) {
      Serial.println("✗ ADXL345 not found");
      while (1) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
      }
    }
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("✓ ADXL345 ready");
  #endif

  #ifdef SENSOR_ACCEL_MPU6050
    if (!mpu.begin()) {
      Serial.println("✗ MPU6050 not found");
      while (1) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
      }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("✓ MPU6050 ready");
  #endif

  // Init clustering
  if (!kmeans_init(&model, FEATURE_DIM, 0.2f)) {
    Serial.println("✗ Model init failed");
    while (1) delay(1000);
  }

  Serial.print("✓ Model ready: K=");
  Serial.print(model.k);
  Serial.print(", dim=");
  Serial.println(FEATURE_DIM);

  #ifdef HAS_WIFI
  if (WiFi.isConnected()) {
    mqtt_connect();
  }
  #endif

  Serial.println("=== Ready ===\n");
  Serial.println("Publishing summaries every 10s");
}

void loop() {
  unsigned long now = millis();

  #ifdef HAS_WIFI
  mqtt.loop();
  #endif

  // Sample at 10 Hz
  if (now - last_sample < SAMPLE_INTERVAL_MS) {
    return;
  }
  last_sample = now;

  // Don't sample if frozen
  if (kmeans_get_state(&model) == STATE_FROZEN) {
    return;
  }

  // Read raw accelerometer
  float ax, ay, az;

  #ifdef SENSOR_ACCEL_ADXL345
    sensors_event_t event;
    accel.getEvent(&event);
    ax = event.acceleration.x;
    ay = event.acceleration.y;
    az = event.acceleration.z;
  #endif

  #ifdef SENSOR_ACCEL_MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
  #endif

  // Extract features
  float features_float[FEATURE_DIM];
  FeatureExtractor::extractInstantaneous(ax, ay, az, features_float);

  // Store in window
  window_samples[window_index][0] = features_float[0];  // rms
  window_samples[window_index][1] = features_float[1];  // peak
  window_samples[window_index][2] = features_float[2];  // crest
  window_index = (window_index + 1) % WINDOW_SIZE;

  // Convert to fixed-point
  fixed_t features[FEATURE_DIM];
  for (int i = 0; i < FEATURE_DIM; i++) {
    features[i] = FLOAT_TO_FIXED(features_float[i]);
  }

  // Update clustering
  int8_t cluster_id = kmeans_update(&model, features);

  // Check if alarm triggered
  if (cluster_id == -1 && kmeans_get_state(&model) == STATE_FROZEN) {
    Serial.println("⚠️  OUTLIER DETECTED - ALARM FROZEN");
    
    #ifdef HAS_WIFI
    if (mqtt.connected()) {
      publish_summary(true);  // Publish alarm
    }
    #endif

    // Flash LED
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
    
    return;
  }

  // Publish summary every 10s
  if (now - last_publish >= PUBLISH_INTERVAL_MS) {
    last_publish = now;

    char label[MAX_LABEL_LENGTH];
    if (cluster_id >= 0) {
      kmeans_get_label(&model, cluster_id, label);
    } else {
      strcpy(label, "normal");
    }

    Serial.print("Summary: C");
    Serial.print(cluster_id);
    Serial.print("(");
    Serial.print(label);
    Serial.print(") | RMS:");
    Serial.print(features_float[0], 2);
    Serial.print(" | K=");
    Serial.println(model.k);

    #ifdef HAS_WIFI
    if (mqtt.connected()) {
      publish_summary(false);
    } else {
      mqtt_connect();
    }
    #endif

    // Activity blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
  }
}