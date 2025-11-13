/**
 * @file streaming_kmeans.c
 * @brief Label-driven incremental clustering
 *
 * Key features:
 * - Starts with K=1 (no pre-training)
 * - Grows clusters when new labels added
 * - EMA centroid updates: c_new = c_old + α(x - c_old)
 * - Constant memory: O(KD)
 */

#include "streaming_kmeans.h"
#include <string.h>
#include <stdlib.h>

// Simple PRNG for initialization
uint32_t rng_state = 12345;

fixed_t rand_fixed(fixed_t min, fixed_t max) {
    rng_state = (1103515245 * rng_state + 12345) & 0x7FFFFFFF;
    fixed_t range = max - min;
    return min + (fixed_t)(((int64_t)range * rng_state) >> 31);
}

bool kmeans_init(kmeans_model_t* model, uint8_t feature_dim, float learning_rate) {
    if (feature_dim > MAX_FEATURES || feature_dim == 0) {
        return false;
    }

    memset(model, 0, sizeof(kmeans_model_t));
    model->k = 1;  // Start with single cluster
    model->feature_dim = feature_dim;
    model->learning_rate = FLOAT_TO_FIXED(learning_rate);

    // Initialize baseline cluster at origin
    memset(model->clusters[0].centroid, 0, feature_dim * sizeof(fixed_t));
    strncpy(model->clusters[0].label, "normal", MAX_LABEL_LENGTH - 1);
    model->clusters[0].label[MAX_LABEL_LENGTH - 1] = '\0';
    model->clusters[0].active = true;
    model->clusters[0].count = 0;

    model->initialized = true;
    return true;
}

static fixed_t distance_squared(const fixed_t* a, const fixed_t* b, uint8_t dim) {
    int64_t sum = 0;
    for (uint8_t i = 0; i < dim; i++) {
        int64_t diff = (int64_t)a[i] - (int64_t)b[i];
        sum += (diff * diff) >> FIXED_POINT_SHIFT;
    }
    return (fixed_t)sum;
}

static uint8_t find_nearest_cluster(const kmeans_model_t* model, const fixed_t* point) {
    uint8_t nearest = 0;
    fixed_t min_dist = distance_squared(point, model->clusters[0].centroid, model->feature_dim);

    for (uint8_t i = 1; i < model->k; i++) {
        if (!model->clusters[i].active) continue;

        fixed_t dist = distance_squared(point, model->clusters[i].centroid, model->feature_dim);
        if (dist < min_dist) {
            min_dist = dist;
            nearest = i;
        }
    }
    return nearest;
}

uint8_t kmeans_update(kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) return 0;

    uint8_t cluster_id = find_nearest_cluster(model, point);
    cluster_t* cluster = &model->clusters[cluster_id];

    // Adaptive learning rate: α = base_lr / (1 + 0.01 × count)
    float decay = 1.0f + 0.01f * cluster->count;
    float alpha_f = FIXED_TO_FLOAT(model->learning_rate) / decay;
    fixed_t alpha = FLOAT_TO_FIXED(alpha_f);

    // Update centroid: c_new = c_old + α(x - c_old)
    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - cluster->centroid[i];
        cluster->centroid[i] += FIXED_MUL(alpha, diff);
    }

    cluster->count++;
    model->total_points++;

    // Update inertia (within-cluster variance)
    fixed_t dist = distance_squared(point, cluster->centroid, model->feature_dim);
    cluster->inertia += FIXED_MUL(alpha, dist - cluster->inertia);

    return cluster_id;
}

uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) return 0;
    return find_nearest_cluster(model, point);
}

bool kmeans_add_cluster(kmeans_model_t* model,
                        const fixed_t* seed_point,
                        const char* label) {
    if (!model->initialized) return false;
    if (model->k >= MAX_CLUSTERS) return false;
    if (!label || strlen(label) == 0) return false;

    // Check for duplicate label
    for (uint8_t i = 0; i < model->k; i++) {
        if (strcmp(model->clusters[i].label, label) == 0) {
            return false;  // Label already exists
        }
    }

    // Create new cluster
    cluster_t* new_cluster = &model->clusters[model->k];
    memcpy(new_cluster->centroid, seed_point,
           model->feature_dim * sizeof(fixed_t));
    strncpy(new_cluster->label, label, MAX_LABEL_LENGTH - 1);
    new_cluster->label[MAX_LABEL_LENGTH - 1] = '\0';
    new_cluster->active = true;
    new_cluster->count = 1;
    new_cluster->inertia = 0;

    model->k++;
    return true;
}

bool kmeans_get_centroid(const kmeans_model_t* model,
                        uint8_t cluster_id,
                        fixed_t* centroid) {
    if (!model->initialized || cluster_id >= model->k) {
        return false;
    }
    memcpy(centroid, model->clusters[cluster_id].centroid,
           model->feature_dim * sizeof(fixed_t));
    return true;
}

bool kmeans_get_label(const kmeans_model_t* model,
                     uint8_t cluster_id,
                     char* label) {
    if (!model->initialized || cluster_id >= model->k) {
        return false;
    }
    strncpy(label, model->clusters[cluster_id].label, MAX_LABEL_LENGTH);
    return true;
}

fixed_t kmeans_inertia(const kmeans_model_t* model) {
    if (!model->initialized) return 0;

    fixed_t total = 0;
    for (uint8_t i = 0; i < model->k; i++) {
        if (model->clusters[i].active) {
            total += model->clusters[i].inertia;
        }
    }
    return total;
}

void kmeans_reset(kmeans_model_t* model) {
    if (!model->initialized) return;

    uint8_t feature_dim = model->feature_dim;
    fixed_t lr = model->learning_rate;

    kmeans_init(model, feature_dim, FIXED_TO_FLOAT(lr));
}

bool kmeans_correct(kmeans_model_t* model,
                    const fixed_t* point,
                    uint8_t old_cluster,
                    uint8_t new_cluster) {
    if (!model->initialized) return false;
    if (old_cluster >= model->k || new_cluster >= model->k) return false;
    if (old_cluster == new_cluster) return true;

    // Repel from old cluster
    cluster_t* old = &model->clusters[old_cluster];
    fixed_t repel_rate = FLOAT_TO_FIXED(0.1f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - old->centroid[i];
        old->centroid[i] -= FIXED_MUL(repel_rate, diff);
    }

    if (old->count > 0) old->count--;

    // Attract to new cluster
    cluster_t* new = &model->clusters[new_cluster];
    fixed_t attract_rate = FLOAT_TO_FIXED(0.2f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - new->centroid[i];
        new->centroid[i] += FIXED_MUL(attract_rate, diff);
    }

    new->count++;
    return true;
}