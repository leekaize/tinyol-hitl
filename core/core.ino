/**
 * @file core.ino
 * @brief Streaming k-means for ESP32-S3 and RP2350
 * 
 * Modes:
 * - MODE_DATASET: Stream CWRU samples via Serial
 * - MODE_SENSOR: Read live ADXL345 data (future)
 *
 * Select board in Arduino IDE:
 * - ESP32: ESP32S3 Dev Module
 * - RP2350: Raspberry Pi Pico 2 W
 */

#include "config.h"

extern "C" {
  #include "streaming_kmeans.h"
}

extern void platform_init();
extern void platform_loop();
extern void platform_blink(uint8_t times);

// Dataset header structure (matches Python streamer)
struct dataset_header {
    uint32_t magic;         // 0x4B4D4541 ("KMEA")
    uint16_t num_samples;
    uint8_t  feature_dim;
    uint8_t  fault_type;    // 0=normal, 1=ball, 2=inner, 3=outer
    uint32_t sample_rate;
    uint32_t reserved;
};

kmeans_model_t model;
dataset_header header;
bool header_received = false;
uint32_t samples_processed = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for Serial to stabilize

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

  Serial.print("Mode: ");
  #ifdef MODE_DATASET
    Serial.println("Dataset Streaming");
  #else
    Serial.println("Sensor Live (not implemented)");
    while (1) platform_blink(10);
  #endif

  platform_init();

  #ifdef MODE_DATASET
  Serial.println("\nWaiting for dataset header...");
  Serial.println("Run: python3 tools/stream_dataset.py <port> <file>");
  #endif

  platform_blink(3);
}

void loop() {
  #ifdef MODE_DATASET
    stream_from_serial();
  #else
    // MODE_SENSOR will go here in future task
  #endif
  
  platform_loop();
}

#ifdef MODE_DATASET
void stream_from_serial() {
  // Read header first (16 bytes)
  if (!header_received && Serial.available() >= sizeof(dataset_header)) {
    Serial.readBytes((char*)&header, sizeof(dataset_header));
    
    // Validate magic number
    if (header.magic != 0x4B4D4541) {
      Serial.printf("ERROR: Invalid magic 0x%08X\n", header.magic);
      platform_blink(10);
      while (1);  // Halt
    }
    
    Serial.println("\n=== Header Received ===");
    Serial.printf("Samples: %u\n", header.num_samples);
    Serial.printf("Features: %u\n", header.feature_dim);
    Serial.printf("Fault type: %u ", header.fault_type);
    switch (header.fault_type) {
      case 0: Serial.println("(Normal)"); break;
      case 1: Serial.println("(Ball fault)"); break;
      case 2: Serial.println("(Inner race)"); break;
      case 3: Serial.println("(Outer race)"); break;
      default: Serial.println("(Unknown)");
    }
    Serial.printf("Sample rate: %u Hz\n", header.sample_rate);
    
    // Initialize k-means with dataset parameters
    // K=4 for 4 fault types, learning_rate=0.2
    if (!kmeans_init(&model, 4, header.feature_dim, 0.2f)) {
      Serial.println("ERROR: Model init failed");
      platform_blink(10);
      while (1);
    }
    
    Serial.printf("Model: %d clusters, %d features, %zu bytes\n",
                  model.k, model.feature_dim, sizeof(kmeans_model_t));
    Serial.println("\n=== Ready for Samples ===");
    
    header_received = true;
    samples_processed = 0;
    
    // Send ACK to Python streamer
    Serial.write(0x06);
    Serial.flush();
  }
  
  // Read samples after header confirmed
  if (header_received) {
    size_t sample_size = sizeof(fixed_t) * header.feature_dim;
    
    if (Serial.available() >= sample_size) {
      fixed_t sample[MAX_FEATURES];
      Serial.readBytes((char*)sample, sample_size);
      
      // Feed to k-means
      uint8_t cluster = kmeans_update(&model, sample);
      
      samples_processed++;
      
      // Log every 10th sample
      if (samples_processed % 10 == 0 || samples_processed == header.num_samples) {
        Serial.printf("Sample %lu/%u -> Cluster %u | ", 
                      samples_processed, header.num_samples, cluster);
        
        // Show centroid distances for debugging
        Serial.print("Distances: [");
        for (uint8_t i = 0; i < model.k; i++) {
          fixed_t dist = 0;
          for (uint8_t j = 0; j < model.feature_dim; j++) {
            fixed_t diff = sample[j] - model.clusters[i].centroid[j];
            dist += FIXED_MUL(diff, diff);
          }
          Serial.printf("%.2f", FIXED_TO_FLOAT(dist));
          if (i < model.k - 1) Serial.print(", ");
        }
        Serial.println("]");
      }
      
      // Send ACK
      Serial.write(0x06);
      Serial.flush();
      
      // Done?
      if (samples_processed >= header.num_samples) {
        Serial.println("\n=== Streaming Complete ===");
        Serial.printf("Total samples: %lu\n", model.total_points);
        
        // Print cluster statistics
        Serial.println("\nCluster Statistics:");
        for (uint8_t i = 0; i < model.k; i++) {
          Serial.printf("  Cluster %u: %u points, inertia=%.4f\n",
                        i, model.clusters[i].count,
                        FIXED_TO_FLOAT(model.clusters[i].inertia));
        }
        
        platform_blink(5);
        
        // Reset for next dataset
        header_received = false;
        samples_processed = 0;
        Serial.println("\nReady for next dataset...");
      }
    }
  }
}
#endif