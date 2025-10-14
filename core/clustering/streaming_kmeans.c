/**
 * @file streaming_kmeans.c
 * @brief Streaming K-Means implementation
 */

#include "streaming_kmeans.h"
#include <string.h>
#include <stdlib.h>

// Simple PRNG for initialization (LCG)
static uint32_t rng_state = 12345;
static fixed_t rand_fixed(fixed_t min, fixed_t max) {
    rng_state = (1103515245 * rng_state + 12345) & 0x7FFFFFFF;
    fixed_t range = max - min;
    return min + (fixed_t)(((int64_t)range * rng_state) >> 31);
}

bool kmeans_init(kmeans_model_t* model, uint8_t k, uint8_t feature_dim, float learning_rate) {
    if (k > MAX_CLUSTERS || feature_dim > MAX_FEATURES || k == 0 || feature_dim == 0) {
        return false;
    }
    
    memset(model, 0, sizeof(kmeans_model_t));
    model->k = k;
    model->feature_dim = feature_dim;
    model->learning_rate = FLOAT_TO_FIXED(learning_rate);
    
    // Initialize centroids with random values in [-1, 1]
    for (uint8_t i = 0; i < k; i++) {
        for (uint8_t j = 0; j < feature_dim; j++) {
            model->clusters[i].centroid[j] = rand_fixed(
                FLOAT_TO_FIXED(-1.0f), 
                FLOAT_TO_FIXED(1.0f)
            );
        }
        model->clusters[i].count = 0;
        model->clusters[i].inertia = 0;
    }
    
    model->initialized = true;
    return true;
}

fixed_t distance_squared(const fixed_t* a, const fixed_t* b, uint8_t dim) {
    int64_t sum = 0;
    for (uint8_t i = 0; i < dim; i++) {
        int64_t diff = (int64_t)a[i] - (int64_t)b[i];
        // Squared distance in fixed-point: (diff^2) >> FIXED_POINT_SHIFT
        sum += (diff * diff) >> FIXED_POINT_SHIFT;
    }
    return (fixed_t)sum;
}

static uint8_t find_nearest_cluster(const kmeans_model_t* model, const fixed_t* point) {
    uint8_t nearest = 0;
    fixed_t min_dist = distance_squared(point, model->clusters[0].centroid, model->feature_dim);
    
    for (uint8_t i = 1; i < model->k; i++) {
        fixed_t dist = distance_squared(point, model->clusters[i].centroid, model->feature_dim);
        if (dist < min_dist) {
            min_dist = dist;
            nearest = i;
        }
    }
    
    return nearest;
}

uint8_t kmeans_update(kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) {
        return 0xFF;  // Error: uninitialized
    }
    
    // Find nearest cluster
    uint8_t cluster_id = find_nearest_cluster(model, point);
    cluster_t* cluster = &model->clusters[cluster_id];
    
    // Adaptive learning rate: α / (1 + count)
    // Decays as cluster sees more points
    cluster->count++;
    fixed_t alpha = FIXED_MUL(
        model->learning_rate,
        FLOAT_TO_FIXED(1.0f / (1.0f + cluster->count * 0.01f))
    );
    
    // Update centroid: c_new = c_old + α(point - c_old)
    fixed_t dist_sq = 0;
    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - cluster->centroid[i];
        cluster->centroid[i] += FIXED_MUL(alpha, diff);
        
        // Track inertia (squared distance to centroid)
        int64_t d = (int64_t)point[i] - (int64_t)cluster->centroid[i];
        dist_sq += (fixed_t)((d * d) >> FIXED_POINT_SHIFT);
    }
    
    // Exponential moving average of inertia
    cluster->inertia = FIXED_MUL(
        FLOAT_TO_FIXED(0.9f), 
        cluster->inertia
    ) + FIXED_MUL(FLOAT_TO_FIXED(0.1f), dist_sq);
    
    model->total_points++;
    return cluster_id;
}

uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) {
        return 0xFF;
    }
    return find_nearest_cluster(model, point);
}

bool kmeans_get_centroid(const kmeans_model_t* model, uint8_t cluster_id, fixed_t* centroid) {
    if (!model->initialized || cluster_id >= model->k) {
        return false;
    }
    memcpy(centroid, model->clusters[cluster_id].centroid, 
           model->feature_dim * sizeof(fixed_t));
    return true;
}

fixed_t kmeans_inertia(const kmeans_model_t* model) {
    if (!model->initialized) {
        return 0;
    }
    
    fixed_t total = 0;
    for (uint8_t i = 0; i < model->k; i++) {
        total += model->clusters[i].inertia;
    }
    return total;
}

void kmeans_reset(kmeans_model_t* model) {
    if (!model->initialized) {
        return;
    }
    
    uint8_t k = model->k;
    uint8_t feature_dim = model->feature_dim;
    fixed_t lr = model->learning_rate;
    
    kmeans_init(model, k, feature_dim, FIXED_TO_FLOAT(lr));
}