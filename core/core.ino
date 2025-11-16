/**
 * @file core.ino
 * @brief Vibration feature extraction + clustering
 *
 * Current config: GY521 (MPU6050) only
 * Features: [rms, peak, crest] = 3D
 */

#include "config.h"
#include "streaming_kmeans.h"
#include "feature_extractor.h"
#include <Wire.h>

#ifndef LED_BUILTIN
  // Default fallbacks for common boards:
  #if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Many ESP32 dev boards use GPIO2 as the built-in LED
    #define LED_BUILTIN 2
  #elif defined(ARDUINO_ARCH_RP2040)
    // Raspberry Pi Pico typically uses GP25
    #define LED_BUILTIN 25
  #else
    // Fall back to the common Arduino LED pin
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
char mqtt_topic_subscribe[64];
#endif

unsigned long last_sample = 0;
const int SAMPLE_INTERVAL_MS = 1000 / SAMPLE_RATE_HZ;

// Feature dimension (compile-time)
const uint8_t FEATURE_DIM = FeatureExtractor::getFeatureDim();

void platform_init() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TinyOL-HITL Starting ===");

  #ifdef ESP32
    Serial.println("Platform: ESP32");
    pinMode(LED_BUILTIN, OUTPUT);
  #elif defined(ARDUINO_ARCH_RP2040)
    Serial.println("Platform: RP2350");
    pinMode(LED_BUILTIN, OUTPUT);
  #endif

  Serial.print("Feature schema v");
  Serial.println(FEATURE_SCHEMA_VERSION);

  // I2C init (platform-specific)
  #ifdef ARDUINO_ARCH_RP2040
    Wire.begin();  // RP2040 uses default pins (GP4/GP5)
  #else
    Wire.begin(I2C_SDA, I2C_SCL);  // ESP32 needs pins
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
      Serial.println(" Failed (offline mode)");
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

  // Add cluster command
  if (!doc["label"].isNull() && !doc["features"].isNull()) {
    const char* label = doc["label"];
    JsonArray features = doc["features"];

    if (features.size() != FEATURE_DIM) {
      Serial.print("Error: Expected ");
      Serial.print(FEATURE_DIM);
      Serial.print(" features, got ");
      Serial.println(features.size());
      return;
    }

    fixed_t point[FEATURE_DIM];
    for (int i = 0; i < FEATURE_DIM; i++) {
      point[i] = FLOAT_TO_FIXED(features[i].as<float>());
    }

    if (kmeans_add_cluster(&model, point, label)) {
      Serial.print("✓ Added cluster: ");
      Serial.print(label);
      Serial.print(" (K=");
      Serial.print(model.k);
      Serial.println(")");

      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("✗ Failed to add cluster");
    }
  }

  // Correction command
  if (!doc["old_cluster"].isNull() && !doc["new_cluster"].isNull()) {
    uint8_t old_cluster = doc["old_cluster"];
    uint8_t new_cluster = doc["new_cluster"];
    JsonArray features = doc["point"];

    if (features.size() != FEATURE_DIM) {
      Serial.println("✗ Correction: wrong feature count");
      return;
    }

    fixed_t point[FEATURE_DIM];
    for (int i = 0; i < FEATURE_DIM; i++) {
      point[i] = FLOAT_TO_FIXED(features[i].as<float>());
    }

    if (kmeans_correct(&model, point, old_cluster, new_cluster)) {
      Serial.print("✓ Corrected: ");
      Serial.print(old_cluster);
      Serial.print(" → ");
      Serial.println(new_cluster);
    }
  }
}

bool mqtt_connect() {
  if (!WiFi.isConnected()) return false;

  Serial.print("MQTT connecting: ");
  Serial.println(MQTT_BROKER);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);

  // Connect with/without credentials
  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS);
  } else {
    connected = mqtt.connect(DEVICE_ID);
  }

  if (connected) {
    Serial.println("✓ MQTT connected");

    snprintf(mqtt_topic_subscribe, sizeof(mqtt_topic_subscribe),
             "tinyol/%s/label", DEVICE_ID);
    mqtt.subscribe(mqtt_topic_subscribe);

    Serial.print("Subscribed: ");
    Serial.println(mqtt_topic_subscribe);

    return true;
  }

  Serial.print("✗ MQTT failed, rc=");
  Serial.println(mqtt.state());
  return false;
}
#endif

void setup() {
  platform_init();

  // Init accelerometer
  #ifdef SENSOR_ACCEL_ADXL345
    if (!accel.begin()) {
      Serial.println("✗ ADXL345 not found");
      Serial.println("Check wiring: VCC→3.3V, GND→GND, SDA→21, SCL→22");
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
      Serial.println("Check wiring: VCC→3.3V, GND→GND, SDA→21, SCL→22");
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

  // Print feature schema
  char names[FEATURE_DIM][32];
  FeatureExtractor::getFeatureNames(names);
  Serial.print("Features: [");
  for (int i = 0; i < FEATURE_DIM; i++) {
    Serial.print(names[i]);
    if (i < FEATURE_DIM - 1) Serial.print(", ");
  }
  Serial.println("]");

  #ifdef HAS_WIFI
  if (WiFi.isConnected()) {
    mqtt_connect();
    snprintf(mqtt_topic_publish, sizeof(mqtt_topic_publish),
             "sensor/%s/data", DEVICE_ID);
  }
  #endif

  Serial.println("=== Ready ===\n");
}

void loop() {
  unsigned long now = millis();

  if (now - last_sample < SAMPLE_INTERVAL_MS) {
    #ifdef HAS_WIFI
    mqtt.loop();
    #endif
    return;
  }
  last_sample = now;

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

  // Convert to fixed-point
  fixed_t features[FEATURE_DIM];
  for (int i = 0; i < FEATURE_DIM; i++) {
    features[i] = FLOAT_TO_FIXED(features_float[i]);
  }

  // Update clustering
  uint8_t cluster_id = kmeans_update(&model, features);

  char label[MAX_LABEL_LENGTH];
  kmeans_get_label(&model, cluster_id, label);

  // Serial output
  Serial.print("C");
  Serial.print(cluster_id);
  Serial.print("(");
  Serial.print(label);
  Serial.print(") | Raw:[");
  Serial.print(ax, 2);
  Serial.print(",");
  Serial.print(ay, 2);
  Serial.print(",");
  Serial.print(az, 2);
  Serial.print("] | F:[");
  for (int i = 0; i < FEATURE_DIM; i++) {
    Serial.print(features_float[i], 2);
    if (i < FEATURE_DIM - 1) Serial.print(",");
  }
  Serial.print("] K=");
  Serial.println(model.k);

  // MQTT publish
  #ifdef HAS_WIFI
  if (WiFi.isConnected()) {
    if (!mqtt.connected()) mqtt_connect();

    if (mqtt.connected()) {
      JsonDocument doc;
      doc["device_id"] = DEVICE_ID;
      doc["cluster"] = cluster_id;
      doc["label"] = label;
      doc["k"] = model.k;
      doc["schema_version"] = FEATURE_SCHEMA_VERSION;
      doc["timestamp"] = now;

      // Feature array
      JsonArray arr = doc["features"].to<JsonArray>();
      for (int i = 0; i < FEATURE_DIM; i++) {
        arr.add(features_float[i]);
      }

      // Raw accel for debugging
      JsonObject raw = doc["raw"].to<JsonObject>();
      raw["ax"] = ax;
      raw["ay"] = ay;
      raw["az"] = az;

      char buffer[384];
      serializeJson(doc, buffer);
      mqtt.publish(mqtt_topic_publish, buffer);
    }
  }
  #endif

  // Activity blink
  digitalWrite(LED_BUILTIN, HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);
}