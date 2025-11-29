/**
 * @file model_storage.h
 * @brief Persistent model storage for TinyOL-HITL
 * 
 * Saves trained clusters to flash. Survives power cycles.
 * 
 * WHEN SAVES HAPPEN:
 *   - Immediately after kmeans_add_cluster() succeeds
 *   - That's it. No periodic saves needed.
 * 
 * WHEN STORAGE CLEARS:
 *   - MQTT reset command: {"reset": true}
 *   - Firmware re-upload
 * 
 * ESP32: Uses Preferences (NVS)
 * RP2350: Uses LittleFS JSON file
 */

#ifndef MODEL_STORAGE_H
#define MODEL_STORAGE_H

#include <Arduino.h>
#include "streaming_kmeans.h"
#include "config.h"

// Storage namespace/filename
#define STORAGE_NAMESPACE "tinyol"
#define STORAGE_FILENAME "/model.bin"

// Magic number to detect valid stored model
#define STORAGE_MAGIC 0x544F4C48  // "TOLH" = TinyOL-HITL

// Version for future compatibility
#define STORAGE_VERSION 1

/**
 * Header stored at beginning of model data
 */
typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t k;
    uint8_t feature_dim;
    uint8_t reserved;
    uint32_t total_points;
    fixed_t outlier_threshold;
    fixed_t learning_rate;
} storage_header_t;

/**
 * Per-cluster data for storage
 */
typedef struct {
    fixed_t centroid[MAX_FEATURES];
    uint32_t count;
    fixed_t inertia;
    char label[MAX_LABEL_LENGTH];
    bool active;
} stored_cluster_t;

// =============================================================================
// Platform-specific implementations
// =============================================================================

#ifdef ESP32

#include <Preferences.h>

class ModelStorage {
private:
    Preferences prefs;
    
public:
    /**
     * Initialize storage subsystem
     */
    bool begin() {
        return prefs.begin(STORAGE_NAMESPACE, false);  // false = read/write
    }
    
    /**
     * Save model to flash
     * @return true if successful
     */
    bool save(const kmeans_model_t* model) {
        if (!model || !model->initialized) return false;
        
        Serial.println("[Storage] Saving model to NVS...");
        
        // Save header
        storage_header_t header;
        header.magic = STORAGE_MAGIC;
        header.version = STORAGE_VERSION;
        header.k = model->k;
        header.feature_dim = model->feature_dim;
        header.reserved = 0;
        header.total_points = model->total_points;
        header.outlier_threshold = model->outlier_threshold;
        header.learning_rate = model->learning_rate;
        
        prefs.putBytes("header", &header, sizeof(header));
        
        // Save each cluster
        for (uint8_t i = 0; i < model->k; i++) {
            char key[16];
            snprintf(key, sizeof(key), "cluster%d", i);
            
            stored_cluster_t sc;
            memcpy(sc.centroid, model->clusters[i].centroid, 
                   model->feature_dim * sizeof(fixed_t));
            sc.count = model->clusters[i].count;
            sc.inertia = model->clusters[i].inertia;
            strncpy(sc.label, model->clusters[i].label, MAX_LABEL_LENGTH);
            sc.active = model->clusters[i].active;
            
            prefs.putBytes(key, &sc, sizeof(sc));
        }
        
        Serial.printf("[Storage] Saved K=%d clusters (%lu points)\n", 
                      model->k, model->total_points);
        return true;
    }
    
    /**
     * Load model from flash
     * @return true if valid model found and loaded
     */
    bool load(kmeans_model_t* model) {
        if (!model) return false;
        
        Serial.println("[Storage] Loading model from NVS...");
        
        // Read header
        storage_header_t header;
        size_t len = prefs.getBytes("header", &header, sizeof(header));
        
        if (len != sizeof(header)) {
            Serial.println("[Storage] No saved model found");
            return false;
        }
        
        if (header.magic != STORAGE_MAGIC) {
            Serial.println("[Storage] Invalid magic number");
            return false;
        }
        
        if (header.version != STORAGE_VERSION) {
            Serial.printf("[Storage] Version mismatch (stored=%d, current=%d)\n",
                          header.version, STORAGE_VERSION);
            return false;
        }
        
        if (header.feature_dim != model->feature_dim) {
            Serial.printf("[Storage] Feature dim mismatch (stored=%d, current=%d)\n",
                          header.feature_dim, model->feature_dim);
            return false;
        }
        
        // Load clusters
        for (uint8_t i = 0; i < header.k; i++) {
            char key[16];
            snprintf(key, sizeof(key), "cluster%d", i);
            
            stored_cluster_t sc;
            len = prefs.getBytes(key, &sc, sizeof(sc));
            
            if (len != sizeof(sc)) {
                Serial.printf("[Storage] Failed to load cluster %d\n", i);
                return false;
            }
            
            memcpy(model->clusters[i].centroid, sc.centroid,
                   model->feature_dim * sizeof(fixed_t));
            model->clusters[i].count = sc.count;
            model->clusters[i].inertia = sc.inertia;
            strncpy(model->clusters[i].label, sc.label, MAX_LABEL_LENGTH);
            model->clusters[i].active = sc.active;
        }
        
        // Update model state
        model->k = header.k;
        model->total_points = header.total_points;
        model->outlier_threshold = header.outlier_threshold;
        model->learning_rate = header.learning_rate;
        model->initialized = true;
        model->state = STATE_NORMAL;
        
        Serial.printf("[Storage] Loaded K=%d clusters (%lu points)\n",
                      model->k, model->total_points);
        return true;
    }
    
    /**
     * Check if valid model exists in storage
     */
    bool hasModel() {
        storage_header_t header;
        size_t len = prefs.getBytes("header", &header, sizeof(header));
        return (len == sizeof(header) && header.magic == STORAGE_MAGIC);
    }
    
    /**
     * Erase all stored model data
     */
    void clear() {
        Serial.println("[Storage] Clearing saved model...");
        prefs.clear();
        Serial.println("[Storage] Model cleared");
    }
    
    /**
     * Get storage stats
     */
    void printStats() {
        Serial.printf("[Storage] Free entries: %d\n", prefs.freeEntries());
    }
};

#elif defined(ARDUINO_ARCH_RP2040)

#include <LittleFS.h>

class ModelStorage {
public:
    bool begin() {
        if (!LittleFS.begin()) {
            Serial.println("[Storage] LittleFS mount failed, formatting...");
            LittleFS.format();
            return LittleFS.begin();
        }
        return true;
    }
    
    bool save(const kmeans_model_t* model) {
        if (!model || !model->initialized) return false;
        
        Serial.println("[Storage] Saving model to LittleFS...");
        
        File file = LittleFS.open(STORAGE_FILENAME, "w");
        if (!file) {
            Serial.println("[Storage] Failed to open file for writing");
            return false;
        }
        
        // Write header
        storage_header_t header;
        header.magic = STORAGE_MAGIC;
        header.version = STORAGE_VERSION;
        header.k = model->k;
        header.feature_dim = model->feature_dim;
        header.reserved = 0;
        header.total_points = model->total_points;
        header.outlier_threshold = model->outlier_threshold;
        header.learning_rate = model->learning_rate;
        
        file.write((uint8_t*)&header, sizeof(header));
        
        // Write clusters
        for (uint8_t i = 0; i < model->k; i++) {
            stored_cluster_t sc;
            memcpy(sc.centroid, model->clusters[i].centroid,
                   model->feature_dim * sizeof(fixed_t));
            sc.count = model->clusters[i].count;
            sc.inertia = model->clusters[i].inertia;
            strncpy(sc.label, model->clusters[i].label, MAX_LABEL_LENGTH);
            sc.active = model->clusters[i].active;
            
            file.write((uint8_t*)&sc, sizeof(sc));
        }
        
        file.close();
        
        Serial.printf("[Storage] Saved K=%d clusters (%lu points)\n",
                      model->k, model->total_points);
        return true;
    }
    
    bool load(kmeans_model_t* model) {
        if (!model) return false;
        
        Serial.println("[Storage] Loading model from LittleFS...");
        
        if (!LittleFS.exists(STORAGE_FILENAME)) {
            Serial.println("[Storage] No saved model found");
            return false;
        }
        
        File file = LittleFS.open(STORAGE_FILENAME, "r");
        if (!file) {
            Serial.println("[Storage] Failed to open file");
            return false;
        }
        
        // Read header
        storage_header_t header;
        if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
            file.close();
            return false;
        }
        
        if (header.magic != STORAGE_MAGIC || header.version != STORAGE_VERSION) {
            Serial.println("[Storage] Invalid or outdated model file");
            file.close();
            return false;
        }
        
        if (header.feature_dim != model->feature_dim) {
            Serial.println("[Storage] Feature dimension mismatch");
            file.close();
            return false;
        }
        
        // Read clusters
        for (uint8_t i = 0; i < header.k; i++) {
            stored_cluster_t sc;
            if (file.read((uint8_t*)&sc, sizeof(sc)) != sizeof(sc)) {
                file.close();
                return false;
            }
            
            memcpy(model->clusters[i].centroid, sc.centroid,
                   model->feature_dim * sizeof(fixed_t));
            model->clusters[i].count = sc.count;
            model->clusters[i].inertia = sc.inertia;
            strncpy(model->clusters[i].label, sc.label, MAX_LABEL_LENGTH);
            model->clusters[i].active = sc.active;
        }
        
        file.close();
        
        model->k = header.k;
        model->total_points = header.total_points;
        model->outlier_threshold = header.outlier_threshold;
        model->learning_rate = header.learning_rate;
        model->initialized = true;
        model->state = STATE_NORMAL;
        
        Serial.printf("[Storage] Loaded K=%d clusters (%lu points)\n",
                      model->k, model->total_points);
        return true;
    }
    
    bool hasModel() {
        if (!LittleFS.exists(STORAGE_FILENAME)) return false;
        
        File file = LittleFS.open(STORAGE_FILENAME, "r");
        if (!file) return false;
        
        storage_header_t header;
        bool valid = (file.read((uint8_t*)&header, sizeof(header)) == sizeof(header))
                     && (header.magic == STORAGE_MAGIC);
        file.close();
        return valid;
    }
    
    void clear() {
        Serial.println("[Storage] Clearing saved model...");
        LittleFS.remove(STORAGE_FILENAME);
        Serial.println("[Storage] Model cleared");
    }
    
    void printStats() {
        FSInfo fs_info;
        LittleFS.info(fs_info);
        Serial.printf("[Storage] Used: %lu / %lu bytes\n", 
                      fs_info.usedBytes, fs_info.totalBytes);
    }
};

#else
#error "No storage implementation for this platform"
#endif

#endif // MODEL_STORAGE_H