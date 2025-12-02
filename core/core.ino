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
const int DEBUG_MS = 5000;    // 1 second debug print
const int WINDOW_SETTLE = 50;  // Skip first 50 samples (5s warmup)
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
      
      mqtt.setBufferSize(2048);  // Was 1024, increase for safety
        
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
  Serial.printf("[MQTT] << %s: ", topic);
  
  // Print raw payload for debug
  char payloadStr[256];
  len = min(len, (unsigned int)255);
  memcpy(payloadStr, payload, len);
  payloadStr[len] = '\0';
  Serial.println(payloadStr);
  
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) {
    Serial.println("[MQTT] JSON parse error");
    return;
  }
  
  // Handle LABEL command
  if (strstr(topic, "/label")) {
    const char* label = doc["label"];
    
    if (!label || strlen(label) == 0) {
      Serial.println("[MQTT] Empty label - ignored");
      return;
    }
    
    Serial.printf("[MQTT] Label: '%s'\n", label);
    
    // Special case: "normal" always retrains cluster 0
    if (strcmp(label, "normal") == 0 && model.k > 0) {
      Serial.println("[MQTT] Retraining 'normal' cluster 0");
      if (kmeans_assign_existing(&model, 0)) {
        storage.save(&model);
        Serial.printf("[MQTT] ✓ Retrained cluster 0 with %d samples\n", model.buffer.count);
      }
      return;
    }
    
    // Check if label already exists
    int existing = -1;
    for (uint8_t i = 0; i < model.k; i++) {
      char existing_label[MAX_LABEL_LENGTH];
      kmeans_get_label(&model, i, existing_label);
      if (strcmp(existing_label, label) == 0) {
        existing = i;
        break;
      }
    }
    
    bool success = false;
    if (existing >= 0) {
      success = kmeans_assign_existing(&model, existing);
      if (success) Serial.printf("[MQTT] ✓ Trained existing cluster %d\n", existing);
    } else {
      success = kmeans_add_cluster(&model, label);
      if (success) Serial.printf("[MQTT] ✓ Created cluster K=%d\n", model.k);
    }
    
    if (success) {
      storage.save(&model);
      for(int i=0; i<3; i++) { 
        digitalWrite(LED_BUILTIN, HIGH); delay(50); 
        digitalWrite(LED_BUILTIN, LOW); delay(50); 
      }
    }
    return;
  }
  
  // Handle DISCARD command
  if (strstr(topic, "/discard")) {
    if (doc["discard"] == true) {
      Serial.println("[MQTT] Discard command");
      kmeans_discard(&model);
      Serial.println("[MQTT] ✓ Discarded");
      mqtt.publish(topic_discard, "{\"discard\":false}");
    }
    return;
  }
  
  // Handle FREEZE command
  if (strstr(topic, "/freeze")) {
    if (doc["freeze"] == true) {
      Serial.println("[MQTT] Freeze command");
      kmeans_request_label(&model);
      Serial.println("[MQTT] ✓ Frozen");
      mqtt.publish(topic_freeze, "{\"freeze\":false}");
    }
    return;
  }
  
  // Handle RESET command
  if (strstr(topic, "/reset")) {
    if (doc["reset"] == true) {
      Serial.println("[MQTT] *** RESET ***");
      storage.clear();
      kmeans_reset(&model);
      Serial.printf("[MQTT] ✓ Reset to K=%d\n", model.k);
      mqtt.publish(topic_reset, "{\"reset\":false}");
      for(int i=0; i<5; i++) { 
        digitalWrite(LED_BUILTIN, HIGH); delay(200); 
        digitalWrite(LED_BUILTIN, LOW); delay(200); 
      }
    }
    return;
  }
  
  // Handle ASSIGN command
  if (strstr(topic, "/assign")) {
    if (doc.containsKey("cluster_id")) {
      uint8_t cid = doc["cluster_id"];
      Serial.printf("[MQTT] Assign to cluster %d\n", cid);
      if (kmeans_assign_existing(&model, cid)) {
        storage.save(&model);
        Serial.println("[MQTT] ✓ Assigned");
        mqtt.publish(topic_assign, "{\"cluster_id\":-1}");
      } else {
        Serial.println("[MQTT] ✗ Assign failed");
      }
    }
    return;
  }
  
  Serial.println("[MQTT] Unknown topic");
}

bool mqttConnect() {
  if (!WiFi.isConnected()) return false;
  if (millis() - lastMqttAttempt < 5000) return false;
  lastMqttAttempt = millis();
  
  Serial.printf("[MQTT] Connecting to %s:%d... ", MQTT_BROKER, MQTT_PORT);
  
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  
  String clientId = String(DEVICE_ID) + "_" + String(random(0xffff), HEX);
  
  bool ok = strlen(MQTT_USER) > 0 
    ? mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)
    : mqtt.connect(clientId.c_str());
  
  if (ok) {
    Serial.println("OK");
    
    // Subscribe with QoS 1 for reliability
    mqtt.subscribe(topic_label, 1);
    mqtt.subscribe(topic_discard, 1);
    mqtt.subscribe(topic_freeze, 1);
    mqtt.subscribe(topic_reset, 1);
    mqtt.subscribe(topic_assign, 1);
    
    Serial.println("[MQTT] Subscribed to all control topics");
    return true;
  }
  
  Serial.printf("FAILED (state=%d)\n", mqtt.state());
  return false;
}

void safePublish(const char* topic, const char* payload) {
  if (strlen(payload) > 1900) {
    Serial.printf("[MQTT] ⚠ Payload too large: %d bytes, truncating\n", strlen(payload));
    return;
  }
  if (!mqtt.connected()) {
    Serial.println("[MQTT] Not connected");
    return;
  }
  mqtt.publish(topic, payload);
}

void publishSummary() {
  if (!mqtt.connected()) {
    Serial.println("[MQTT] Not connected, skipping publish");
    return;
  }

  // --- STATE STRING (handle all states) ---
  system_state_t state = kmeans_get_state(&model);
  const char* stateStr;
  switch(state) {
    case STATE_BOOTSTRAP:    stateStr = "BOOTSTRAP"; break;
    case STATE_NORMAL:       stateStr = "NORMAL"; break;
    case STATE_ALARM:        stateStr = "ALARM"; break;
    case STATE_WAITING_LABEL: stateStr = "WAITING_LABEL"; break;
    default:                 stateStr = "UNKNOWN"; break;
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

  // --- BOOLEAN FLAGS (derived from state, not separate) ---
  bool isAlarm = (state == STATE_ALARM) || (state == STATE_WAITING_LABEL);
  bool isWaiting = (state == STATE_WAITING_LABEL);
  bool isBootstrap = (state == STATE_BOOTSTRAP);
  
  // Get current cluster label
  char currentLabel[MAX_LABEL_LENGTH] = "unknown";
  int currentCluster = -1;
  if (state == STATE_NORMAL && model.k > 0) {
    currentCluster = 0;  // In normal state, last assigned cluster
    kmeans_get_label(&model, 0, currentLabel);
  }
  
  // --- BUILD JSON ---
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["state"] = stateStr;
  doc["alarm_active"] = isAlarm;
  doc["waiting_label"] = isWaiting;
  doc["motor_running"] = kmeans_is_motor_running(&model);
  
  doc["cluster"] = currentCluster;
  doc["label"] = currentLabel;
  doc["k"] = model.k;
  doc["total_points"] = model.total_points;
  
  doc["vib_rms_avg"] = vib_rms_sum / limit;
  doc["vib_rms_max"] = vib_rms_max;
  doc["vib_peak_max"] = vib_peak_max;
  doc["vib_crest_avg"] = vib_crest_sum / limit;
  
  #ifdef USE_CURRENT
  doc["current_rms_avg"] = i_rms_sum / limit;
  doc["current_rms_max"] = i_rms_max;
  #endif

  doc["baseline"] = FeatureExtractor::getBaseline();
  doc["buffer_samples"] = kmeans_get_buffer_size(&model);
  doc["sample_count"] = limit;
  doc["timestamp"] = millis();

  char buf[1024];
  size_t len = serializeJson(doc, buf);
  
  Serial.printf("[MQTT] >> %s (%d bytes)\n", topic_data, len);

  if (mqtt.publish(topic_data, buf)) {
    Serial.println("[MQTT] ✓ Published");
  } else {
    Serial.println("[MQTT] ✗ Publish failed");
  }
}
#endif

const char* stateToString(system_state_t s) {
  switch(s) {
    case STATE_BOOTSTRAP:     return "BOOTSTRAP";
    case STATE_NORMAL:        return "NORMAL";
    case STATE_ALARM:         return "ALARM";
    case STATE_WAITING_LABEL: return "WAITING_LABEL";
    default:                  return "UNKNOWN";
  }
}

void logStateChange(const char* reason, system_state_t before, system_state_t after) {
  if (before != after) {
    Serial.printf("\n>>> STATE CHANGE: %s → %s (reason: %s)\n\n", 
                  stateToString(before), stateToString(after), reason);
  }
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
  static uint32_t loopCount = 0;
  loopCount++;
  
  // Crash watchdog
  // if (loopCount % 100 == 0) {
  //   Serial.printf("[Watchdog] Loop %lu, Free heap: %d\n", 
  //                 loopCount, ESP.getFreeHeap());
  // }
  
  // Check for low memory
  if (ESP.getFreeHeap() < 10000) {
    Serial.println("[CRITICAL] Low heap! Skipping operations");
    delay(1000);
    return;
  }
  
  unsigned long now = millis();

  #ifdef HAS_WIFI
    if (!mqtt.connected()) {
      mqttConnect();
    }
    mqtt.loop();

    static unsigned long lastMqttCheck = 0;
    if (now - lastMqttCheck >= 5000) {
      lastMqttCheck = now;
      Serial.printf("[MQTT] Connected: %s, State: %d\n", 
                    mqtt.connected() ? "YES" : "NO", 
                    mqtt.state());
    }
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

  if (!kmeans_is_motor_running(&model)) {
    Serial.println("[Loop] Motor OFF - skipping clustering");
    
    // Still publish status but don't cluster
    if (now - lastPublish >= PUBLISH_MS) {
        lastPublish = now;
        #ifdef HAS_WIFI
          publishSummary();
        #endif
    }
    return;
  }

  // Motor stopped = always baseline (cluster 0)
  if (!kmeans_is_motor_running(&model) && model.state == STATE_NORMAL) {
    // Force assign to cluster 0 when motor stopped
    // This prevents false alarms during shutdown
    Serial.println("[Motor] Stopped - baseline mode");
  }

  // Skip clustering entirely when motor stopped
  if (!kmeans_is_motor_running(&model)) {
    if (now - lastDebug >= DEBUG_MS) {
      lastDebug = now;
      Serial.println("⏸️  Motor stopped - skipping clustering");
    }
    return;  // Don't run kmeans_update when motor off
  }

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

  system_state_t stateBefore = kmeans_get_state(&model);
  Serial.printf("[Loop] Before update: state=%s, motor=%s, k=%d\n",
                stateToString(stateBefore),
                kmeans_is_motor_running(&model) ? "ON" : "OFF",
                model.k);

  int8_t clusterId = kmeans_update(&model, featuresFixed);

  system_state_t stateAfter = kmeans_get_state(&model);
  logStateChange("kmeans_update", stateBefore, stateAfter);

  Serial.printf("[Loop] After update: cluster=%d, alarm=%s, buffer=%d\n",
                clusterId,
                kmeans_is_alarm_active(&model) ? "YES" : "no",
                kmeans_get_buffer_size(&model));

  // LED feedback
  state = kmeans_get_state(&model);
  if (state == STATE_ALARM) {
    digitalWrite(LED_BUILTIN, (millis() / 200) % 2); // Blink fast
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Debug output every second
  if (now - lastDebug >= DEBUG_MS) {
    lastDebug = now;
    
    system_state_t state = kmeans_get_state(&model);
    
    Serial.println("========================================");
    Serial.printf("STATE: %s | K=%d | Motor: %s\n", 
                  stateToString(state), model.k,
                  kmeans_is_motor_running(&model) ? "RUNNING" : "STOPPED");
    Serial.printf("Cluster: %d | Alarm: %s | Frozen: %s\n",
                  clusterId,
                  kmeans_is_alarm_active(&model) ? "YES" : "no",
                  model.buffer.frozen ? "YES" : "no");
    Serial.printf("Buffer: %d samples | Normal streak: %d/%d\n",
                  model.buffer.count,
                  model.normal_streak,
                  ALARM_CLEAR_SAMPLES);
    Serial.printf("Features: RMS=%.2f Peak=%.2f Crest=%.2f\n", 
                  lastRms, lastPeak, lastCrest);
    Serial.printf("Threshold: %.2f | Last distance: %.2f\n",
                  FIXED_TO_FLOAT(model.outlier_threshold),
                  FIXED_TO_FLOAT(model.last_distance));
    Serial.println("========================================");
  }

  // Publish every 10 seconds
  if (now - lastPublish >= PUBLISH_MS) {
    lastPublish = now;
    #ifdef HAS_WIFI
      publishSummary();
    #endif
  }
}