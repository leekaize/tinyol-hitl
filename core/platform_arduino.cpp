#ifdef PLATFORM_ARDUINO

#include <EEPROM.h>
#include "config.h"

// Optional: WiFi shield support
// #include <WiFi101.h> or <ESP8266WiFi.h>

void platform_init() {
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Arduino platform (no WiFi by default)");
  Serial.println("Storage ready (EEPROM)");

  // If WiFi shield attached, uncomment:
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void platform_loop() {
  // WiFi reconnect if shield present
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