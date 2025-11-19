/**
 * @file streaming_kmeans.h
 * @brief Label-driven incremental clustering with freeze-on-alarm
 *
 * Features:
 * - Starts with K=1 (baseline "normal")
 * - Grows dynamically when operator labels new faults
 * - Outlier detection triggers alarm state
 * - Freeze-on-alarm workflow for operator inspection
 * - Memory: K × D × 4 bytes + ring buffer
 * - Precision: Q16.16 fixed-point
 */

#ifndef STREAMING_KMEANS_H
#define STREAMING_KMEANS_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define MAX_CLUSTERS 16
#define MAX_FEATURES 64
#define MAX_LABEL_LENGTH 32
#define FIXED_POINT_SHIFT 16  // Q16.16 format
#define RING_BUFFER_SIZE 100  // 10 seconds @ 10 Hz

// Fixed-point type (range: ±32,768)
typedef int32_t fixed_t;

// Conversion macros
#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * (1 << FIXED_POINT_SHIFT)))
#define FIXED_TO_FLOAT(x) ((float)(x) / (1 << FIXED_POINT_SHIFT))
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_POINT_SHIFT)

// System states
typedef enum {
    STATE_NORMAL,   // Normal operation, collecting samples
    STATE_ALARM,    // Outlier detected, transitioning to freeze
    STATE_FROZEN    // Waiting for operator action
} system_state_t;

// Ring buffer for freeze-on-alarm
typedef struct {
    fixed_t samples[RING_BUFFER_SIZE][MAX_FEATURES];
    uint16_t head;          // Write pointer
    uint16_t count;         // Samples stored
    bool frozen;            // Buffer immutable
} ring_buffer_t;

// Cluster state
typedef struct {
    fixed_t centroid[MAX_FEATURES];  // Cluster center
    uint32_t count;                   // Points assigned
    fixed_t inertia;                  // Within-cluster variance
    char label[MAX_LABEL_LENGTH];     // "normal", "ball_fault", etc
    bool active;                      // Is this cluster in use?
} cluster_t;

// Model state
typedef struct {
    cluster_t clusters[MAX_CLUSTERS];
    uint8_t k;                        // Current number of clusters
    uint8_t feature_dim;              // Feature dimension
    fixed_t learning_rate;            // Base learning rate (α)
    uint32_t total_points;            // Total points processed
    bool initialized;                 // Model ready flag
    
    // Alarm state
    system_state_t state;             // Current system state
    ring_buffer_t buffer;             // Sample buffer for freeze
    fixed_t outlier_threshold;        // Distance threshold (× cluster radius)
    fixed_t last_distance;            // Distance of last sample
} kmeans_model_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize model with K=1 baseline cluster
 * @param model Model to initialize
 * @param feature_dim Feature dimension (1-64)
 * @param learning_rate Base learning rate (0.01-0.5 typical)
 * @return true if successful
 */
bool kmeans_init(kmeans_model_t* model, uint8_t feature_dim, float learning_rate);

/**
 * Process single data point (assign + update centroid)
 * @param model Model to update
 * @param point Feature vector (length = feature_dim)
 * @return Assigned cluster ID (0 to k-1), or -1 if frozen
 */
int8_t kmeans_update(kmeans_model_t* model, const fixed_t* point);

/**
 * Predict cluster without updating model
 * @param model Model to query
 * @param point Feature vector
 * @return Predicted cluster ID
 */
uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point);

/**
 * Add new cluster with operator-provided label
 * Uses frozen buffer as seed samples
 * @param model Model to update
 * @param label Fault label (e.g. "outer_race_fault")
 * @return true if cluster added
 */
bool kmeans_add_cluster(kmeans_model_t* model, const char* label);

/**
 * Check if current sample is an outlier
 * @param model Model to query
 * @param point Feature vector
 * @return true if distance > threshold × cluster_radius
 */
bool kmeans_is_outlier(const kmeans_model_t* model, const fixed_t* point);

/**
 * Trigger alarm state (freeze buffer)
 * @param model Model to update
 */
void kmeans_alarm(kmeans_model_t* model);

/**
 * Discard frozen buffer and resume normal operation
 * @param model Model to update
 */
void kmeans_discard(kmeans_model_t* model);

/**
 * Get current system state
 * @param model Model to query
 * @return Current state (NORMAL, ALARM, FROZEN)
 */
system_state_t kmeans_get_state(const kmeans_model_t* model);

/**
 * Get frozen buffer size
 * @param model Model to query
 * @return Number of samples in frozen buffer (0 if not frozen)
 */
uint16_t kmeans_get_buffer_size(const kmeans_model_t* model);

/**
 * Get centroid coordinates
 * @param model Model to query
 * @param cluster_id Cluster index
 * @param centroid Output buffer (must have feature_dim elements)
 * @return true if successful
 */
bool kmeans_get_centroid(const kmeans_model_t* model,
                        uint8_t cluster_id,
                        fixed_t* centroid);

/**
 * Get cluster label
 * @param model Model to query
 * @param cluster_id Cluster index
 * @param label Output buffer (must have MAX_LABEL_LENGTH bytes)
 * @return true if successful
 */
bool kmeans_get_label(const kmeans_model_t* model,
                     uint8_t cluster_id,
                     char* label);

/**
 * Get total inertia (sum of within-cluster variances)
 * @param model Model to query
 * @return Total inertia (fixed-point)
 */
fixed_t kmeans_inertia(const kmeans_model_t* model);

/**
 * Reset model to K=1 baseline
 * @param model Model to reset
 */
void kmeans_reset(kmeans_model_t* model);

/**
 * Apply human correction (move point from wrong cluster to correct cluster)
 * @param model Model to update
 * @param point Misclassified sample
 * @param old_cluster Current (wrong) assignment
 * @param new_cluster Correct assignment (per operator)
 * @return true if successful
 */
bool kmeans_correct(kmeans_model_t* model,
                    const fixed_t* point,
                    uint8_t old_cluster,
                    uint8_t new_cluster);

/**
 * Set outlier detection threshold
 * @param model Model to update
 * @param multiplier Threshold as multiple of cluster radius (default: 2.0)
 */
void kmeans_set_threshold(kmeans_model_t* model, float multiplier);

#ifdef __cplusplus
}
#endif

#endif // STREAMING_KMEANS_H