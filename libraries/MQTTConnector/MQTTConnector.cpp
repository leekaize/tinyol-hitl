#include "MQTTConnector.h"

MQTTConnector::MQTTConnector(const char* broker, uint16_t port, const char* client_id)
  #ifdef HAS_WIFI
  : mqtt(wifi),
  #endif
    broker(broker), client_id(client_id) {
  #ifdef HAS_WIFI
  mqtt.setServer(broker, port);
  #endif
}

bool MQTTConnector::connect() {
  #ifdef HAS_WIFI
  if (mqtt.connect(client_id)) {
    Serial.println("MQTT connected");
    return true;
  }
  return false;
  #else
  return false;
  #endif
}

bool MQTTConnector::publishCluster(uint8_t id, float* features, uint8_t dim) {
  #ifdef HAS_WIFI
  snprintf(topic, sizeof(topic), "sensor/%s/cluster", client_id);
  int len = snprintf(payload, sizeof(payload), "{\"cluster\":%d,\"features\":[", id);
  for (uint8_t i = 0; i < dim; i++) {
    len += snprintf(payload + len, sizeof(payload) - len, "%.3f", features[i]);
    if (i < dim - 1) len += snprintf(payload + len, sizeof(payload) - len, ",");
  }
  snprintf(payload + len, sizeof(payload) - len, "]}");
  return mqtt.publish(topic, payload);
  #else
  return false;
  #endif
}

void MQTTConnector::loop() {
  #ifdef HAS_WIFI
  if (!mqtt.connected()) connect();
  mqtt.loop();
  #endif
}