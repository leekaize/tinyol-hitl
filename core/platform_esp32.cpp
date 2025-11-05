#ifdef PLATFORM_ESP32

#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

Preferences prefs;

void platform_init() {
  pinMode(LED_PIN, OUTPUT);

  #ifdef HAS_WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected: %s\n", WiFi.localIP().toString().c_str());
  #endif

  prefs.begin("tinyol", false);
  Serial.println("Storage ready (NVS)");
}

void platform_loop() {
  #ifdef HAS_WIFI
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  #endif
}

void platform_blink(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

#endif