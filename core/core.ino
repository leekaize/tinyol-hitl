/**
 * @file core.ino
 * @brief TinyOL-HITL: Label-driven clustering with vibration + current
 * 
 * Features:
 * - 7D feature vector [vib_rms, vib_peak, vib_crest, i1, i2, i3, i_rms]
 * - Gravity Compensation (High-pass filter on Accel)
 * - Freeze-on-alarm (Human-in-the-loop) workflow
 * - MQTT integration for SCADA/Dashboard
 * - Cross-platform (ESP32/RP2350)
 * - **NEW: Persistent model storage (survives power cycles)**
 */

#include "config.h"
#include "streaming_kmeans.h"
#include "feature_extractor.h"
#include "model_storage.h"  // NEW: Persistence
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
ModelStorage storage;  // NEW: Persistence handler

#ifdef USE_CURRENT
  CurrentSensor currentSensor;
#endif

#ifdef HAS_WIFI
  WiFiClient wifiClient;
  PubSubClient mqtt(wifiClient);
  char topic_data[64];
  char topic_label[64];
  char topic_discard[64];
  char topic_freeze[64];
  char topic_reset[64];
  char topic_assign[64];  // NEW: Assign to existing cluster
  unsigned long lastMqttAttempt = 0;
#endif

// Windowing for Statistics (Averaging before publish)
const int WINDOW_SIZE = 100; // 100 samples @ 10Hz = 10 seconds
float window_features[WINDOW_SIZE][FEATURE_DIM];
int window_idx = 0;

// Timing
const int SAMPLE_MS = 100;    // 10 Hz sampling
const int PUBLISH_MS = 10000; // 10 seconds publish cycle
const int DEBUG_MS = 1000;    // 1 second debug print
unsigned long lastSample = 0;
unsigned long lastPublish = 0;
unsigned long lastDebug = 0;

// Instantaneous stats for debug
float lastRawAx = 0, lastRawAy = 0, lastRawAz = 0;
float lastRms = 0, lastPeak = 0, lastCrest = 0;
int sampleCount = 0;

// =============================================================================
// Setup
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for Serial
  
  Serial.println("\n========================================");
  Serial.println("TinyOL-HITL v3.2 (Persistent Storage)");
  Serial.println("========================================");
  
  #ifdef ESP32
    Serial.println("Platform: ESP32");
  #elif defined(ARDUINO_ARCH_RP2040)
    Serial.println("Platform: RP2350");
  #endif
  
  Serial.printf("Features: %dD\n", FEATURE_DIM);
  Serial.printf("Model size: %d bytes\n", sizeof(kmeans_model_t));
  
  pinMode(LED_BUILTIN, OUTPUT);

  // NEW: Initialize storage FIRST
  Serial.print("[Storage] Initializing... ");
  if (!storage.begin()) {
    Serial.println("FAILED!");
  } else {
    Serial.println("OK");
    storage.printStats();
  }

  // I2C
  #ifdef ARDUINO_ARCH_RP2040
    Wire.begin();
    Serial.println("[I2C] RP2040 default pins");
  #else
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("[I2C] ESP32 SDA=%d SCL=%d\n", I2C_SDA, I2C_SCL);
  #endif

  // Accelerometer
  #ifdef SENSOR_ACCEL_MPU6050
    Serial.print("[MPU6050] Initializing... ");
    if (!mpu.begin()) {
      Serial.println("FAILED!");
      while (1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("OK");
  #endif

  #ifdef SENSOR_ACCEL_ADXL345
    Serial.print("[ADXL345] Initializing... ");
    if (!accel.begin()) {
      Serial.println("FAILED!");
      while (1) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); }
    }
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("OK");
  #endif

  // Current sensors
  #ifdef USE_CURRENT
    Serial.println("[Current] Calibrating...");
    currentSensor.calibrate();
  #else
    Serial.println("[Current] Not enabled");
  #endif

  // Model initialization with persistence
  Serial.print("[Model] Initializing... ");
  if (!kmeans_init(&model, FEATURE_DIM, LEARNING_RATE)) {
    Serial.println("FAILED!");
    while (1) delay(1000);
  }
  Serial.printf("OK (K=%d)\n", model.k);

  // NEW: Try to load saved model
  if (storage.hasModel()) {
    Serial.println("[Model] Found saved model, loading...");
    if (storage.load(&model)) {
      Serial.printf("[Model] ✓ Restored K=%d clusters\n", model.k);
      // List loaded clusters
      for (uint8_t i = 0; i < model.k; i++) {
        char label[MAX_LABEL_LENGTH];
        kmeans_get_label(&model, i, label);
        Serial.printf("  C%d: \"%s\" (%lu samples)\n", 
                      i, label, model.clusters[i].count);
      }
    } else {
      Serial.println("[Model] Load failed, using fresh model");
    }
  } else {
    Serial.println("[Model] No saved model, starting fresh (K=1)");
  }
  
  // WiFi
  #ifdef HAS_WIFI
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf(" OK (%s)\n", WiFi.localIP().toString().c_str());
      
      // Increase MQTT Buffer size for large JSON payloads
      mqtt.setBufferSize(1024);

      // Setup Topics
      snprintf(topic_data, sizeof(topic_data), "sensor/%s/data", DEVICE_ID);
      snprintf(topic_label, sizeof(topic_label), "tinyol/%s/label", DEVICE_ID);
      snprintf(topic_discard, sizeof(topic_discard), "tinyol/%s/discard", DEVICE_ID);
      snprintf(topic_freeze, sizeof(topic_freeze), "tinyol/%s/freeze", DEVICE_ID);
      snprintf(topic_reset, sizeof(topic_reset), "tinyol/%s/reset", DEVICE_ID);
      snprintf(topic_assign, sizeof(topic_assign), "tinyol/%s/assign", DEVICE_ID);
      
      Serial.println("[MQTT] Topics Configured:");
      Serial.printf("  DATA:    %s\n", topic_data);
      Serial.printf("  LABEL:   %s\n", topic_label);
      Serial.printf("  DISCARD: %s\n", topic_discard);
      Serial.printf("  RESET:   %s\n", topic_reset);
      Serial.printf("  ASSIGN:  %s\n", topic_assign);
    } else {
      Serial.println(" FAILED");
    }
  #endif

  Serial.println("\n=== READY ===\n");
  Serial.println("Commands via MQTT:");
  Serial.println("  label:   {\"label\":\"fault_name\"}  - Create NEW cluster K++");
  Serial.println("  assign:  {\"cluster_id\":1}         - Train EXISTING cluster");
  Serial.println("  discard: {\"discard\":true}         - Discard alarm");
  Serial.println("  freeze:  {\"freeze\":true}          - Manual freeze");
  Serial.println("  reset:   {\"reset\":true}           - Reset model to K=1");
  Serial.println("");
}

// =============================================================================
// MQTT
// =============================================================================

#ifdef HAS_WIFI
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.printf("[MQTT] Received on %s: ", topic);
  
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) {
    Serial.println("JSON parse error");
    return;
  }
  
  // Print payload
  serializeJson(doc, Serial);
  Serial.println();

  // Handle LABEL command
  if (strstr(topic, "/label")) {
    const char* label = doc["label"];
    if (label) {
      Serial.printf("[MQTT] Label command: '%s'\n", label);
      if (kmeans_add_cluster(&model, label)) {
        Serial.printf("[MQTT] ✓ Created cluster K=%d\n", model.k);
        
        // Save immediately after creating cluster
        storage.save(&model);
        
        // Visual feedback
        for(int i=0; i<3; i++) { 
          digitalWrite(LED_BUILTIN, HIGH); delay(50); 
          digitalWrite(LED_BUILTIN, LOW); delay(50); 
        }
      } else {
        Serial.println("[MQTT] ✗ Label failed (not in WAITING_LABEL state?)");
      }
    }
  }
  
  // Handle DISCARD command
  if (strstr(topic, "/discard")) {
    if (doc["discard"] == true) {
      Serial.println("[MQTT] Discard command");
      kmeans_discard(&model);
      Serial.println("[MQTT] ✓ Discarded");
    }
  }
  
  // Handle FREEZE command
  if (strstr(topic, "/freeze")) {
    if (doc["freeze"] == true) {
      Serial.println("[MQTT] Freeze command");
      kmeans_request_label(&model);
      Serial.println("[MQTT] ✓ Frozen (Wait Label)");
    }
  }
  
  // Handle RESET command
  if (strstr(topic, "/reset")) {
    if (doc["reset"] == true) {
      Serial.println("[MQTT] *** RESET COMMAND ***");
      Serial.println("[MQTT] Clearing saved model and resetting to K=1...");
      
      // Clear storage
      storage.clear();
      
      // Re-initialize model
      kmeans_reset(&model);
      
      Serial.printf("[MQTT] ✓ Model reset to K=%d\n", model.k);
      
      // Long blink to indicate reset
      for(int i=0; i<5; i++) { 
        digitalWrite(LED_BUILTIN, HIGH); delay(200); 
        digitalWrite(LED_BUILTIN, LOW); delay(200); 
      }
    }
  }

  // Handle ASSIGN command (assign buffer to existing cluster)
  if (strstr(topic, "/assign")) {
    if (doc.containsKey("cluster_id")) {
      uint8_t cluster_id = doc["cluster_id"];
      Serial.printf("[MQTT] Assign command: cluster %d\n", cluster_id);
      if (kmeans_assign_existing(&model, cluster_id)) {
        Serial.printf("[MQTT] ✓ Assigned buffer to cluster %d\n", cluster_id);
        
        // Save after assignment (cluster updated)
        storage.save(&model);
        
        // Visual feedback
        for(int i=0; i<2; i++) { 
          digitalWrite(LED_BUILTIN, HIGH); delay(100); 
          digitalWrite(LED_BUILTIN, LOW); delay(100); 
        }
      } else {
        Serial.println("[MQTT] ✗ Assign failed (not in WAITING_LABEL state or invalid cluster?)");
      }
    }
  }
}

bool mqttConnect() {
  if (!WiFi.isConnected()) return false;
  
  // Rate limit connection attempts
  if (millis() - lastMqttAttempt < 5000) return false;
  lastMqttAttempt = millis();
  
  Serial.printf("[MQTT] Connecting to %s:%d... ", MQTT_BROKER, MQTT_PORT);
  
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  
  bool ok = strlen(MQTT_USER) > 0 
    ? mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)
    : mqtt.connect(DEVICE_ID);
  
  if (ok) {
    Serial.println("OK");
    
    mqtt.subscribe(topic_label);
    mqtt.subscribe(topic_discard);
    mqtt.subscribe(topic_freeze);
    mqtt.subscribe(topic_reset);
    mqtt.subscribe(topic_assign);
    
    return true;
  }
  
  Serial.printf("FAILED (state=%d)\n", mqtt.state());
  return false;
}

void publishSummary() {
  if (!mqtt.connected()) {
    Serial.println("[MQTT] Not connected, skipping publish");
    return;
  }

  // --- COMPUTE STATISTICS ---
  float vib_rms_sum = 0, vib_rms_max = 0;
  float vib_peak_sum = 0, vib_peak_max = 0;
  float vib_crest_sum = 0, vib_crest_max = 0;
  #ifdef USE_CURRENT
  float i_rms_sum = 0, i_rms_max = 0;
  #endif

  int n = (window_idx > 0) ? window_idx : WINDOW_SIZE;
  int limit = (sampleCount < WINDOW_SIZE) ? sampleCount : n;
  if (limit == 0) limit = 1;

  for (int i = 0; i < limit; i++) {
    vib_rms_sum += window_features[i][0];
    vib_peak_sum += window_features[i][1];
    vib_crest_sum += window_features[i][2];
    
    if (window_features[i][0] > vib_rms_max) vib_rms_max = window_features[i][0];
    if (window_features[i][1] > vib_peak_max) vib_peak_max = window_features[i][1];
    if (window_features[i][2] > vib_crest_max) vib_crest_max = window_features[i][2];
    
    #ifdef USE_CURRENT
    float i_val = window_features[i][FEATURE_DIM-1]; 
    i_rms_sum += i_val;
    if (i_val > i_rms_max) i_rms_max = i_val;
    #endif
  }

  // --- PREPARE JSON ---
  system_state_t state = kmeans_get_state(&model);
  const char* stateStr = (state == STATE_NORMAL) ? "NORMAL" :
                         (state == STATE_ALARM) ? "ALARM" : "WAITING_LABEL";
  
  // Get current cluster label
  char currentLabel[MAX_LABEL_LENGTH] = "unknown";
  if (!kmeans_is_alarm_active(&model) && model.k > 0) {
    kmeans_get_label(&model, 0, currentLabel);  // Default to cluster 0
  }
  
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["state"] = stateStr;
  doc["alarm_active"] = kmeans_is_alarm_active(&model);
  doc["waiting_label"] = kmeans_is_waiting_label(&model);
  doc["motor_running"] = kmeans_is_motor_running(&model);
  
  doc["cluster"] = kmeans_is_alarm_active(&model) ? -1 : 0;
  doc["label"] = currentLabel;
  doc["k"] = model.k;
  doc["total_points"] = model.total_points;  // NEW: Include total training points
  
  // Statistical Features
  doc["vib_rms_avg"] = vib_rms_sum / limit;
  doc["vib_rms_max"] = vib_rms_max;
  doc["vib_peak_max"] = vib_peak_max;
  doc["vib_crest_avg"] = vib_crest_sum / limit;
  
  #ifdef USE_CURRENT
  doc["current_rms_avg"] = i_rms_sum / limit;
  doc["current_rms_max"] = i_rms_max;
  #endif

  // System Health
  doc["baseline"] = FeatureExtractor::getBaseline();
  doc["buffer_samples"] = kmeans_get_buffer_size(&model);
  doc["sample_count"] = limit;
  doc["timestamp"] = millis();

  char buf[1024];
  size_t len = serializeJson(doc, buf);
  
  Serial.printf("[MQTT] Publishing to %s (%d bytes)...\n", topic_data, len);

  if (mqtt.publish(topic_data, buf)) {
    Serial.println("[MQTT] ✓ Success");
  } else {
    Serial.println("[MQTT] ✗ FAILED");
  }
}
#endif

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
  unsigned long now = millis();

  #ifdef HAS_WIFI
    if (!mqtt.connected()) {
      mqttConnect();
    }
    mqtt.loop();
  #endif

  // Sample at 10 Hz
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;
  sampleCount++;

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
  
  lastRawAx = ax; lastRawAy = ay; lastRawAz = az;

  // Read current
  float i1 = 0, i2 = 0, i3 = 0;
  #ifdef USE_CURRENT
    currentSensor.read(&i1, &i2, &i3);
  #endif

  // Extract features (Gravity Compensated)
  float features[FEATURE_DIM];
  FeatureExtractor::extractSimple(ax, ay, az, i1, i2, i3, features);
  
  lastRms = features[0];
  lastPeak = features[1];
  lastCrest = features[2];

  // Store in window buffer for statistics
  for (int i = 0; i < FEATURE_DIM; i++) {
    window_features[window_idx][i] = features[i];
  }
  window_idx = (window_idx + 1) % WINDOW_SIZE;

  // Update motor status (Idle detection)
  fixed_t rmsFixed = FLOAT_TO_FIXED(features[0]);
  #ifdef USE_CURRENT
    fixed_t currentFixed = FLOAT_TO_FIXED(features[FEATURE_DIM - 1]);
  #else
    fixed_t currentFixed = 0;
  #endif
  kmeans_update_motor_status(&model, rmsFixed, currentFixed);

  // Skip K-Means update if Frozen (Waiting for Label)
  system_state_t state = kmeans_get_state(&model);
  if (state == STATE_WAITING_LABEL) {
    if (now - lastDebug >= DEBUG_MS) {
      lastDebug = now;
      Serial.println("⏸️  WAITING_LABEL - send label or discard via MQTT");
      for(int i=0;i<2;i++) { 
        digitalWrite(LED_BUILTIN, HIGH); delay(50); 
        digitalWrite(LED_BUILTIN, LOW); delay(50); 
      }
    }
    return;
  }

  // Convert to fixed-point and update model
  fixed_t featuresFixed[FEATURE_DIM];
  for (int i = 0; i < FEATURE_DIM; i++) {
    featuresFixed[i] = FLOAT_TO_FIXED(features[i]);
  }

  int8_t clusterId = kmeans_update(&model, featuresFixed);

  // LED feedback
  state = kmeans_get_state(&model);
  if (state == STATE_ALARM) {
    digitalWrite(LED_BUILTIN, (millis() / 200) % 2); // Blink fast
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Debug output every 1 second
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    
    const char* stateStr = (state == STATE_NORMAL) ? "NORMAL" :
                           (state == STATE_ALARM) ? "ALARM" : "WAIT";
    
    Serial.println("----------------------------------------");
    Serial.printf("State: %s | K=%d | Motor: %s | Points: %lu\n", 
                  stateStr, model.k,
                  kmeans_is_motor_running(&model) ? "ON" : "OFF",
                  model.total_points);
    Serial.printf("Features: RMS=%.3f Peak=%.3f Crest=%.2f\n", 
                  lastRms, lastPeak, lastCrest);
    #ifdef USE_CURRENT
      Serial.printf("Current: L1=%.2f L2=%.2f L3=%.2f\n", i1, i2, i3);
    #endif
    Serial.printf("Cluster: %d | Alarm: %s | Buffer: %d\n",
                  clusterId,
                  kmeans_is_alarm_active(&model) ? "YES" : "no",
                  kmeans_get_buffer_size(&model));
  }

  // Publish every 10 seconds
  if (now - lastPublish >= PUBLISH_MS) {
    lastPublish = now;
    #ifdef HAS_WIFI
      publishSummary();
    #endif
  }
}