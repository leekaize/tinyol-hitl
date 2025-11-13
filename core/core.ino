/**
 * @file core.ino
 * @brief Label-driven incremental clustering for predictive maintenance
 * 
 * Hardware: ESP32-S3 or RP2350 (Pico 2 W)
 * Sensor: ADXL345 accelerometer (I2C)
 * Protocol: MQTT pub/sub
 */

#include "config.h"
#include "streaming_kmeans.h"
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>

#ifdef HAS_WIFI
  #include <PubSubClient.h>
  #include <ArduinoJson.h>
  #ifdef ESP32
    #include <WiFi.h>
  #else
    #include <WiFi.h>  // RP2350 uses same WiFi.h
  #endif
#endif

// Globals
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
kmeans_model_t model;

#ifdef HAS_WIFI
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char mqtt_topic_publish[64];
char mqtt_topic_subscribe[64];
#endif

unsigned long last_sample = 0;
const int SAMPLE_INTERVAL_MS = 100;  // 10 Hz

// Platform init
void platform_init() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== TinyOL-HITL Starting ===");
  
  #ifdef ESP32
    Serial.println("Platform: ESP32-S3");
    pinMode(LED_BUILTIN, OUTPUT);
  #elif defined(ARDUINO_ARCH_RP2040)
    Serial.println("Platform: RP2350");
    pinMode(LED_BUILTIN, OUTPUT);
  #endif
  
  // I2C init
  Wire.begin();
  
  // WiFi init
  #ifdef HAS_WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" Connected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println(" Failed. Operating offline.");
    }
  #endif
}

// MQTT callback
#ifdef HAS_WIFI
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check if this is a label command
  if (!doc["label"].isNull() && !doc["feature"].isNull()) {
    const char* label = doc["label"];
    JsonArray features = doc["features"];
    
    if (features.size() != 3) {
      Serial.println("Error: Expected 3 features");
      return;
    }
    
    fixed_t point[3];
    for (int i = 0; i < 3; i++) {
      point[i] = FLOAT_TO_FIXED(features[i].as<float>());
    }
    
    if (kmeans_add_cluster(&model, point, label)) {
      Serial.print("Added cluster: ");
      Serial.print(label);
      Serial.print(" (K=");
      Serial.print(model.k);
      Serial.println(")");
      
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("Failed to add cluster (duplicate label or K >= MAX)");
    }
  }
  
  // Check if this is a correction command
  if (!doc["old_cluster"].isNull() && !doc["new_cluster"].isNull()) {
    uint8_t old_cluster = doc["old_cluster"];
    uint8_t new_cluster = doc["new_cluster"];
    JsonArray features = doc["point"];
    
    if (features.size() != 3) {
      Serial.println("Error: Expected 3 features in point");
      return;
    }
    
    fixed_t point[3];
    for (int i = 0; i < 3; i++) {
      point[i] = FLOAT_TO_FIXED(features[i].as<float>());
    }
    
    if (kmeans_correct(&model, point, old_cluster, new_cluster)) {
      Serial.print("Corrected: ");
      Serial.print(old_cluster);
      Serial.print(" → ");
      Serial.println(new_cluster);
    }
  }
}

bool mqtt_connect() {
  if (!WiFi.isConnected()) return false;
  
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(MQTT_BROKER);
  
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);
  
  if (mqtt.connect(DEVICE_ID)) {
    Serial.println("MQTT connected");
    
    // Subscribe to label commands
    snprintf(mqtt_topic_subscribe, sizeof(mqtt_topic_subscribe),
             "tinyol/%s/label", DEVICE_ID);
    mqtt.subscribe(mqtt_topic_subscribe);
    
    Serial.print("Subscribed to: ");
    Serial.println(mqtt_topic_subscribe);
    
    return true;
  }
  
  Serial.print("MQTT connection failed, rc=");
  Serial.println(mqtt.state());
  return false;
}
#endif

void setup() {
  platform_init();
  
  // Init accelerometer
  if (!accel.begin()) {
    Serial.println("ERROR: ADXL345 not found!");
    Serial.println("Check wiring:");
    Serial.println("  VCC → 3.3V");
    Serial.println("  GND → GND");
    Serial.println("  SDA → GPIO21 (ESP32) or GP4 (RP2350)");
    Serial.println("  SCL → GPIO22 (ESP32) or GP5 (RP2350)");
    while (1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
  
  accel.setRange(ADXL345_RANGE_16_G);
  Serial.println("ADXL345 initialized");
  
  // Init clustering model
  if (!kmeans_init(&model, 3, 0.2f)) {  // 3D accel, 0.2 learning rate
    Serial.println("ERROR: Model init failed");
    while (1) delay(1000);
  }
  
  Serial.print("Model initialized: K=");
  Serial.print(model.k);
  Serial.println(" (baseline)");
  
  // Connect MQTT
  #ifdef HAS_WIFI
  if (WiFi.isConnected()) {
    mqtt_connect();
    
    snprintf(mqtt_topic_publish, sizeof(mqtt_topic_publish),
             "sensor/%s/cluster", DEVICE_ID);
  }
  #endif
  
  Serial.println("=== System Ready ===");
}

void loop() {
  unsigned long now = millis();
  
  // Sample at 10 Hz
  if (now - last_sample < SAMPLE_INTERVAL_MS) {
    #ifdef HAS_WIFI
    mqtt.loop();
    #endif
    return;
  }
  last_sample = now;
  
  // Read sensor
  sensors_event_t event;
  accel.getEvent(&event);
  
  // Convert to fixed-point
  fixed_t features[3] = {
    FLOAT_TO_FIXED(event.acceleration.x),
    FLOAT_TO_FIXED(event.acceleration.y),
    FLOAT_TO_FIXED(event.acceleration.z)
  };
  
  // Update clustering
  uint8_t cluster_id = kmeans_update(&model, features);
  
  // Get label
  char label[MAX_LABEL_LENGTH];
  kmeans_get_label(&model, cluster_id, label);
  
  // Serial output
  Serial.print("Cluster: ");
  Serial.print(cluster_id);
  Serial.print(" (");
  Serial.print(label);
  Serial.print(") | Accel: [");
  Serial.print(event.acceleration.x, 2);
  Serial.print(", ");
  Serial.print(event.acceleration.y, 2);
  Serial.print(", ");
  Serial.print(event.acceleration.z, 2);
  Serial.print("] | K=");
  Serial.println(model.k);
  
  // MQTT publish
  #ifdef HAS_WIFI
  if (WiFi.isConnected()) {
    if (!mqtt.connected()) {
      mqtt_connect();
    }
    
    if (mqtt.connected()) {
      JsonDocument doc;
      doc["cluster"] = cluster_id;
      doc["label"] = label;
      doc["k"] = model.k;
      
      JsonArray arr = doc["features"].to<JsonArray>();
      arr.add(event.acceleration.x);
      arr.add(event.acceleration.y);
      arr.add(event.acceleration.z);
      
      doc["timestamp"] = now;
      
      char buffer[256];
      serializeJson(doc, buffer);
      
      mqtt.publish(mqtt_topic_publish, buffer);
    }
  }
  #endif
  
  // Blink LED on activity
  digitalWrite(LED_BUILTIN, HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);
}