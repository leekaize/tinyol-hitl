/**
 * @file streaming_kmeans.h
 * @brief Label-driven incremental clustering for edge devices
 *
 * Features:
 * - Starts with K=1 (baseline "normal")
 * - Grows dynamically when operator labels new faults
 * - Memory: K × D × 4 bytes + metadata
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

// Fixed-point type (range: ±32,768)
typedef int32_t fixed_t;

// Conversion macros
#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * (1 << FIXED_POINT_SHIFT)))
#define FIXED_TO_FLOAT(x) ((float)(x) / (1 << FIXED_POINT_SHIFT))
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_POINT_SHIFT)

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
 * @return Assigned cluster ID (0 to k-1)
 */
uint8_t kmeans_update(kmeans_model_t* model, const fixed_t* point);

/**
 * Predict cluster without updating model
 * @param model Model to query
 * @param point Feature vector
 * @return Predicted cluster ID
 */
uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point);

/**
 * Add new cluster with operator-provided label
 * @param model Model to update
 * @param seed_point First sample of new fault type
 * @param label Fault label (e.g. "outer_race_fault")
 * @return true if cluster added, false if K >= MAX_CLUSTERS or duplicate label
 */
bool kmeans_add_cluster(kmeans_model_t* model,
                        const fixed_t* seed_point,
                        const char* label);

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

#ifdef __cplusplus
}
#endif

#endif // STREAMING_KMEANS_H