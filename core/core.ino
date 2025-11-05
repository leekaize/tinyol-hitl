/**
 * @file core.ino
 * @brief Streaming k-means for ESP32-S3 and RP2350
 *
 * Select board in Arduino IDE:
 * - ESP32: ESP32S3 Dev Module
 * - RP2350: Raspberry Pi Pico 2 W
 */

#include "config.h"
#include "streaming_kmeans.h"

extern void platform_init();
extern void platform_loop();
extern void platform_blink(uint8_t times);

kmeans_model_t model;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n=== TinyOL: Streaming K-Means ===");
  Serial.print("Platform: ");
  #ifdef PLATFORM_ESP32
    Serial.println("ESP32-S3");
  #elif defined(PLATFORM_RP2350)
    Serial.println("RP2350 (Pico 2 W)");
  #else
    Serial.println("ERROR: Unsupported platform");
    while (1) platform_blink(10);
  #endif

  platform_init();

  if (!kmeans_init(&model, 3, 2, 0.2f)) {
    Serial.println("ERROR: Model init failed");
    while (1) platform_blink(10);
  }

  Serial.printf("Model: %d clusters, %d features, %zu bytes\n",
                model.k, model.feature_dim, sizeof(kmeans_model_t));
  platform_blink(3);
}

void loop() {
  platform_loop();
  delay(100);
}