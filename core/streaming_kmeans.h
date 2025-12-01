/**
 * @file streaming_kmeans.h
 * @brief Label-driven clustering with proper alarm/freeze semantics
 * 
 * State machine:
 *   NORMAL → [outlier] → ALARM (alert active, still sampling)
 *   ALARM → [motor stops OR button] → WAITING_LABEL (frozen)
 *   ALARM → [returns to normal] → NORMAL (auto-clear)
 *   WAITING_LABEL → [label] → NORMAL (K++)
 *   WAITING_LABEL → [discard] → NORMAL
 */

#ifndef STREAMING_KMEANS_H
#define STREAMING_KMEANS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CLUSTERS 16
#define MAX_FEATURES 64
#define MAX_LABEL_LENGTH 32
#define FIXED_POINT_SHIFT 16
#define RING_BUFFER_SIZE 100

typedef int32_t fixed_t;

#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * (1 << FIXED_POINT_SHIFT)))
#define FIXED_TO_FLOAT(x) ((float)(x) / (1 << FIXED_POINT_SHIFT))
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_POINT_SHIFT)

/**
 * System states:
 * - NORMAL: No alarm, sampling active
 * - ALARM: Outlier detected, motor running, still sampling (alert banner)
 * - WAITING_LABEL: Frozen, motor stopped OR button pressed, ready for label
 */
typedef enum {
    STATE_NORMAL,        // Green - all good
    STATE_ALARM,         // Red banner - outlier detected, motor running
    STATE_WAITING_LABEL  // Red + frozen - ready for operator input
} system_state_t;

// Thresholds for motor-stopped detection
#define IDLE_RMS_THRESHOLD FLOAT_TO_FIXED(0.5f)      // m/s²
#define IDLE_CURRENT_THRESHOLD FLOAT_TO_FIXED(0.1f)  // Amps
#define IDLE_CONSECUTIVE_SAMPLES 10                   // 1 second @ 10Hz
#define ALARM_CLEAR_SAMPLES 30                        // 3 seconds of normal = auto-clear

typedef struct {
    fixed_t samples[RING_BUFFER_SIZE][MAX_FEATURES];
    uint16_t head;
    uint16_t count;
    bool frozen;
} ring_buffer_t;

typedef struct {
    fixed_t centroid[MAX_FEATURES];
    uint32_t count;
    fixed_t inertia;
    char label[MAX_LABEL_LENGTH];
    bool active;
} cluster_t;

typedef struct {
    cluster_t clusters[MAX_CLUSTERS];
    uint8_t k;
    uint8_t feature_dim;
    fixed_t learning_rate;
    uint32_t total_points;
    bool initialized;
    
    // State machine
    system_state_t state;
    ring_buffer_t buffer;
    fixed_t outlier_threshold;
    fixed_t last_distance;
    
    // Alarm tracking
    bool alarm_active;           // Red banner visible
    bool waiting_label;          // Frozen, ready for input
    uint16_t alarm_sample_count; // Samples since alarm triggered
    uint16_t normal_streak;      // Consecutive normal samples (for auto-clear)
    
    // Motor status detection
    uint8_t idle_count;          // Consecutive idle samples
    fixed_t last_rms;
    fixed_t last_current;
    bool motor_running;
} kmeans_model_t;

#ifdef __cplusplus
extern "C" {
#endif

// Core API
bool kmeans_init(kmeans_model_t* model, uint8_t feature_dim, float learning_rate);
int8_t kmeans_update(kmeans_model_t* model, const fixed_t* point);
uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point);

// Alarm handling
bool kmeans_add_cluster(kmeans_model_t* model, const char* label);
void kmeans_discard(kmeans_model_t* model);
// Assign buffered anomaly to existing cluster (no K++)
bool kmeans_assign_existing(kmeans_model_t* model, uint8_t cluster_id);
void kmeans_request_label(kmeans_model_t* model);  // Manual button press

// Status queries
system_state_t kmeans_get_state(const kmeans_model_t* model);
bool kmeans_is_alarm_active(const kmeans_model_t* model);
bool kmeans_is_waiting_label(const kmeans_model_t* model);
bool kmeans_is_motor_running(const kmeans_model_t* model);
uint16_t kmeans_get_buffer_size(const kmeans_model_t* model);

// Motor status update (call every sample)
void kmeans_update_motor_status(kmeans_model_t* model, fixed_t rms, fixed_t current);

// Utilities
bool kmeans_get_centroid(const kmeans_model_t* model, uint8_t cluster_id, fixed_t* centroid);
bool kmeans_get_label(const kmeans_model_t* model, uint8_t cluster_id, char* label);
fixed_t kmeans_inertia(const kmeans_model_t* model);
void kmeans_reset(kmeans_model_t* model);
bool kmeans_correct(kmeans_model_t* model, const fixed_t* point, uint8_t old_cluster, uint8_t new_cluster);
void kmeans_set_threshold(kmeans_model_t* model, float multiplier);

// Legacy compatibility
bool kmeans_is_outlier(const kmeans_model_t* model, const fixed_t* point);

#ifdef __cplusplus
}
#endif

#endif