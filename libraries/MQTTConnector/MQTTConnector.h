#ifndef MQTT_CONNECTOR_H
#define MQTT_CONNECTOR_H

#include <PubSubClient.h>

#ifdef HAS_WIFI
  #include <WiFiClient.h>
#else
  #warning "No WiFi - MQTT disabled"
#endif

class MQTTConnector {
public:
  MQTTConnector(const char* broker, uint16_t port, const char* client_id);
  bool connect();
  bool publishCluster(uint8_t id, float* features, uint8_t dim);
  void loop();

private:
  #ifdef HAS_WIFI
  WiFiClient wifi;
  PubSubClient mqtt;
  #endif
  const char* broker;
  const char* client_id;
  char topic[128];
  char payload[256];
};

#endif