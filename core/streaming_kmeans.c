/**
 * @file streaming_kmeans.c
 * @brief Label-driven incremental clustering with freeze-on-alarm
 * 
 * Algorithm: Exponential moving average (EMA) k-means
 * Memory: K × D × 4 bytes (max 4.2KB for K=16, D=64)
 * Precision: Q16.16 fixed-point
 * 
 * Key features:
 * - Starts with K=1 (no pre-training)
 * - Operator labels outliers to create new clusters
 * - Freeze-on-alarm workflow for physical inspection
 * - Idle detection persists alarms across shifts
 */

#include "streaming_kmeans.h"
#include <string.h>
#include <stdlib.h>

// Simple PRNG for initialization (not cryptographically secure)
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
    model->k = 1;  // Start with single "normal" cluster
    model->feature_dim = feature_dim;
    model->learning_rate = FLOAT_TO_FIXED(learning_rate);
    model->state = STATE_NORMAL;
    model->outlier_threshold = FLOAT_TO_FIXED(2.0f);  // 2× cluster radius
    model->idle_count = 0;
    model->last_rms = 0;
    model->last_current = 0;

    // Initialize baseline cluster at origin
    memset(model->clusters[0].centroid, 0, feature_dim * sizeof(fixed_t));
    strncpy(model->clusters[0].label, "normal", MAX_LABEL_LENGTH - 1);
    model->clusters[0].label[MAX_LABEL_LENGTH - 1] = '\0';
    model->clusters[0].active = true;
    model->clusters[0].count = 0;
    model->clusters[0].inertia = FLOAT_TO_FIXED(1.0f);  // Fixed baseline for stable threshold

    // Initialize ring buffer
    model->buffer.head = 0;
    model->buffer.count = 0;
    model->buffer.frozen = false;

    model->initialized = true;
    return true;
}

/**
 * Compute squared Euclidean distance (avoids sqrt for speed)
 * Distance² = Σ(a[i] - b[i])²
 */
static fixed_t distance_squared(const fixed_t* a, const fixed_t* b, uint8_t dim) {
    int64_t sum = 0;
    for (uint8_t i = 0; i < dim; i++) {
        int64_t diff = (int64_t)a[i] - (int64_t)b[i];
        sum += (diff * diff) >> FIXED_POINT_SHIFT;
    }
    return (fixed_t)sum;
}

/**
 * Find nearest cluster to point
 * @param out_distance Optional output for distance to nearest cluster
 * @return Cluster ID (0 to k-1)
 */
static uint8_t find_nearest_cluster(const kmeans_model_t* model, const fixed_t* point, fixed_t* out_distance) {
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
    
    if (out_distance) {
        *out_distance = min_dist;
    }
    return nearest;
}

/**
 * Add sample to ring buffer (circular overwrite)
 * Skipped if buffer is frozen
 */
static void buffer_add_sample(ring_buffer_t* buffer, const fixed_t* point, uint8_t feature_dim) {
    if (buffer->frozen) return;

    memcpy(buffer->samples[buffer->head], point, feature_dim * sizeof(fixed_t));
    
    buffer->head = (buffer->head + 1) % RING_BUFFER_SIZE;
    if (buffer->count < RING_BUFFER_SIZE) {
        buffer->count++;
    }
}

int8_t kmeans_update(kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) return -1;
    if (model->state == STATE_FROZEN || model->state == STATE_FROZEN_IDLE) return -1;

    // Add to ring buffer for alarm detection
    buffer_add_sample(&model->buffer, point, model->feature_dim);

    // Find nearest cluster
    fixed_t distance;
    uint8_t cluster_id = find_nearest_cluster(model, point, &distance);
    model->last_distance = distance;

    // Check outlier after buffer has baseline (10 samples = 1 second @ 10Hz)
    if (model->buffer.count >= 10 && kmeans_is_outlier(model, point)) {
        kmeans_alarm(model);
        return -1;  // Return -1 when frozen
    }

    cluster_t* cluster = &model->clusters[cluster_id];

    // Update centroid using exponential moving average (EMA)
    // α = learning_rate / (1 + 0.01 × count)
    // c_new = c_old + α(x - c_old)
    float decay = 1.0f + 0.01f * cluster->count;
    float alpha_f = FIXED_TO_FLOAT(model->learning_rate) / decay;
    fixed_t alpha = FLOAT_TO_FIXED(alpha_f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - cluster->centroid[i];
        cluster->centroid[i] += FIXED_MUL(alpha, diff);
    }
    
    // Update inertia (within-cluster variance) using EMA
    cluster->inertia += FIXED_MUL(alpha, distance - cluster->inertia);
    cluster->count++;
    model->total_points++;

    return cluster_id;
}

uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) return 0;
    return find_nearest_cluster(model, point, NULL);
}

/**
 * Check if point is an outlier
 * Outlier = distance > threshold × cluster_radius
 * Default threshold = 2.0
 */
bool kmeans_is_outlier(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized || model->k == 0) return false;

    fixed_t distance;
    uint8_t nearest = find_nearest_cluster(model, point, &distance);
    
    fixed_t radius = model->clusters[nearest].inertia;
    if (radius == 0) {
        radius = FLOAT_TO_FIXED(1.0f);
    }

    fixed_t threshold = FIXED_MUL(model->outlier_threshold, radius);
    return distance > threshold;
}

/**
 * Trigger alarm state - freezes buffer and stops sampling
 * Operator must label or discard to resume
 */
void kmeans_alarm(kmeans_model_t* model) {
    if (model->state != STATE_NORMAL) return;

    model->state = STATE_FROZEN;
    model->buffer.frozen = true;
}

/**
 * Discard frozen buffer (false alarm)
 * Clears buffer and resumes normal sampling
 */
void kmeans_discard(kmeans_model_t* model) {
    if (model->state != STATE_FROZEN && model->state != STATE_FROZEN_IDLE) return;

    model->state = STATE_NORMAL;
    model->buffer.frozen = false;
    model->buffer.head = 0;
    model->buffer.count = 0;
}

/**
 * Add new cluster from frozen buffer (operator labeled outlier)
 * Uses last sample (outlier trigger point) as centroid
 * @param label Fault name from operator (e.g. "bearing_outer_race")
 * @return true if cluster created, false if failed
 */
bool kmeans_add_cluster(kmeans_model_t* model, const char* label) {
    if (!model->initialized) return false;
    if (model->k >= MAX_CLUSTERS) return false;
    if (!label || strlen(label) == 0) return false;
    if (model->state != STATE_FROZEN && model->state != STATE_FROZEN_IDLE) return false;
    if (model->buffer.count == 0) return false;

    // Check duplicate label
    for (uint8_t i = 0; i < model->k; i++) {
        if (strcmp(model->clusters[i].label, label) == 0) {
            return false;
        }
    }

    cluster_t* new_cluster = &model->clusters[model->k];

    // Use last sample (outlier that triggered alarm) as centroid
    // Operator labels based on what they see NOW, not buffer average
    uint16_t last_idx = (model->buffer.head > 0) ? 
                        model->buffer.head - 1 : 
                        model->buffer.count - 1;

    memcpy(new_cluster->centroid, 
           model->buffer.samples[last_idx], 
           model->feature_dim * sizeof(fixed_t));

    strncpy(new_cluster->label, label, MAX_LABEL_LENGTH - 1);
    new_cluster->label[MAX_LABEL_LENGTH - 1] = '\0';
    new_cluster->active = true;
    new_cluster->count = 1;
    new_cluster->inertia = FLOAT_TO_FIXED(1.0f);  // Fixed baseline

    model->k++;

    kmeans_discard(model);  // Clear buffer and resume

    return true;
}

system_state_t kmeans_get_state(const kmeans_model_t* model) {
    return model->state;
}

uint16_t kmeans_get_buffer_size(const kmeans_model_t* model) {
    return model->buffer.frozen ? model->buffer.count : 0;
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

    // Repel from old
    cluster_t* old = &model->clusters[old_cluster];
    fixed_t repel_rate = FLOAT_TO_FIXED(0.1f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - old->centroid[i];
        old->centroid[i] -= FIXED_MUL(repel_rate, diff);
    }

    if (old->count > 0) old->count--;

    // Attract to new
    cluster_t* new = &model->clusters[new_cluster];
    fixed_t attract_rate = FLOAT_TO_FIXED(0.2f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - new->centroid[i];
        new->centroid[i] += FIXED_MUL(attract_rate, diff);
    }

    new->count++;
    return true;
}

void kmeans_set_threshold(kmeans_model_t* model, float multiplier) {
    if (!model->initialized) return;
    if (multiplier < 1.0f) multiplier = 1.0f;
    if (multiplier > 5.0f) multiplier = 5.0f;
    
    model->outlier_threshold = FLOAT_TO_FIXED(multiplier);
}

void kmeans_update_idle(kmeans_model_t* model, fixed_t rms, fixed_t current) {
    if (!model->initialized) return;
    
    model->last_rms = rms;
    model->last_current = current;
    
    bool is_idle_now = (rms < IDLE_RMS_THRESHOLD);
    
    if (current > 0) {
        is_idle_now = is_idle_now && (current < IDLE_CURRENT_THRESHOLD);
    }
    
    if (is_idle_now) {
        model->idle_count++;
        
        if (model->idle_count >= IDLE_CONSECUTIVE_SAMPLES) {
            if (model->state == STATE_FROZEN) {
                model->state = STATE_FROZEN_IDLE;
            }
        }
    } else {
        model->idle_count = 0;
        
        if (model->state == STATE_FROZEN_IDLE) {
            model->state = STATE_FROZEN;
        }
    }
}

bool kmeans_is_idle(const kmeans_model_t* model) {
    if (!model->initialized) return false;
    return model->state == STATE_FROZEN_IDLE;
}