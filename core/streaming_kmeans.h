/**
 * @file streaming_kmeans.h
 * @brief Streaming K-Means for RP2350 (<100KB RAM)
 *
 * Memory: K × feature_dim × 4 bytes + metadata
 * Update: Exponential moving average per point
 * Distance: Squared Euclidean (no sqrt overhead)
 */

#ifndef STREAMING_KMEANS_H
#define STREAMING_KMEANS_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define MAX_CLUSTERS 16
#define MAX_FEATURES 64
#define FIXED_POINT_SHIFT 16  // Q16.16 format

// Fixed-point type
typedef int32_t fixed_t;

// Macros for fixed-point conversion
#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * (1 << FIXED_POINT_SHIFT)))
#define FIXED_TO_FLOAT(x) ((float)(x) / (1 << FIXED_POINT_SHIFT))
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_POINT_SHIFT)

// Cluster state
typedef struct {
    fixed_t centroid[MAX_FEATURES];  // Cluster center
    uint32_t count;                   // Points assigned
    fixed_t inertia;                  // Within-cluster variance
} cluster_t;

// Model state
typedef struct {
    cluster_t clusters[MAX_CLUSTERS];
    uint8_t k;                        // Number of clusters
    uint8_t feature_dim;              // Feature dimension
    fixed_t learning_rate;            // Base learning rate (α)
    uint32_t total_points;            // Total points processed
    bool initialized;                 // Model ready flag
} kmeans_model_t;

// API Functions

/**
 * Initialize model with random centroids
 * @param model Model to initialize
 * @param k Number of clusters
 * @param feature_dim Feature dimension
 * @param learning_rate Base learning rate (0.01 to 0.5 typical)
 * @return true if successful
 */
bool kmeans_init(kmeans_model_t* model, uint8_t k, uint8_t feature_dim, float learning_rate);

/**
 * Process single data point
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
 * Get cluster centroid
 * @param model Model to query
 * @param cluster_id Cluster index
 * @param centroid Output buffer (length = feature_dim)
 * @return true if valid cluster_id
 */
bool kmeans_get_centroid(const kmeans_model_t* model, uint8_t cluster_id, fixed_t* centroid);

/**
 * Calculate total inertia (sum of squared distances)
 * @param model Model to evaluate
 * @return Total inertia value
 */
fixed_t kmeans_inertia(const kmeans_model_t* model);

/**
 * Reset model to initial state
 * @param model Model to reset
 */
void kmeans_reset(kmeans_model_t* model);

/**
 * Apply human-in-the-loop correction
 * @param model Model to update
 * @param point Sample to relabel (feature vector)
 * @param old_cluster Current cluster assignment
 * @param new_cluster Correct cluster (human label)
 * @return true on success, false if invalid parameters
 */
bool kmeans_correct(kmeans_model_t* model,
                    const fixed_t* point,
                    uint8_t old_cluster,
                    uint8_t new_cluster);

// Helper: Squared Euclidean distance (fixed-point)
fixed_t distance_squared(const fixed_t* a, const fixed_t* b, uint8_t dim);

#endif // STREAMING_KMEANS_H