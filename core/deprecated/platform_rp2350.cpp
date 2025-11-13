/**
 * @file platform_rp2350.cpp
 * @brief RP2350 (Pico 2 W) platform implementation
 */

#include "config.h"

#ifdef PLATFORM_RP2350

#include <WiFi.h>
#include <LittleFS.h>

void platform_init() {
  pinMode(LED_PIN, OUTPUT);

  #ifdef HAS_WIFI
  Serial.print("WiFi connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected: %s\n", WiFi.localIP().toString().c_str());
  #endif

  if (LittleFS.begin()) {
    Serial.println("Storage ready (LittleFS)");
  } else {
    Serial.println("WARNING: LittleFS mount failed");
  }
}

void platform_loop() {
  #ifdef HAS_WIFI
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
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

#endif  // PLATFORM_RP2350