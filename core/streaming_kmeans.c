/**
 * @file streaming_kmeans.c
 * @brief Label-driven incremental clustering
 * 
 * Key behaviors:
 * - NORMAL: Sample, update centroids, check outliers
 * - ALARM: Outlier detected, motor running, keep sampling (just alert)
 * - WAITING_LABEL: Motor stopped OR button pressed, frozen for labeling
 */

#include "streaming_kmeans.h"
#include <string.h>
#include <stdlib.h>

// PRNG for initialization
// static uint32_t rng_state = 12345;

// static fixed_t rand_fixed(fixed_t min, fixed_t max) {
//     rng_state = (1103515245 * rng_state + 12345) & 0x7FFFFFFF;
//     fixed_t range = max - min;
//     return min + (fixed_t)(((int64_t)range * rng_state) >> 31);
// }

bool kmeans_init(kmeans_model_t* model, uint8_t feature_dim, float learning_rate) {
    if (feature_dim > MAX_FEATURES || feature_dim == 0) return false;

    memset(model, 0, sizeof(kmeans_model_t));
    model->k = 1;
    model->feature_dim = feature_dim;
    model->learning_rate = FLOAT_TO_FIXED(learning_rate);
    model->state = STATE_NORMAL;
    model->outlier_threshold = FLOAT_TO_FIXED(2.0f);
    
    // Alarm state
    model->alarm_active = false;
    model->waiting_label = false;
    model->alarm_sample_count = 0;
    model->normal_streak = 0;
    
    // Motor status
    model->idle_count = 0;
    model->motor_running = true;  // Assume running initially

    // Initialize baseline cluster
    memset(model->clusters[0].centroid, 0, feature_dim * sizeof(fixed_t));
    strncpy(model->clusters[0].label, "normal", MAX_LABEL_LENGTH - 1);
    model->clusters[0].active = true;
    model->clusters[0].count = 0;
    model->clusters[0].inertia = FLOAT_TO_FIXED(1.0f);

    // Ring buffer
    model->buffer.head = 0;
    model->buffer.count = 0;
    model->buffer.frozen = false;

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
    
    if (out_distance) *out_distance = min_dist;
    return nearest;
}

static void buffer_add_sample(ring_buffer_t* buffer, const fixed_t* point, uint8_t feature_dim) {
    if (buffer->frozen) return;

    memcpy(buffer->samples[buffer->head], point, feature_dim * sizeof(fixed_t));
    buffer->head = (buffer->head + 1) % RING_BUFFER_SIZE;
    if (buffer->count < RING_BUFFER_SIZE) buffer->count++;
}

bool kmeans_is_outlier(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized || model->k == 0) return false;

    fixed_t distance;
    uint8_t nearest = find_nearest_cluster(model, point, &distance);
    
    fixed_t radius = model->clusters[nearest].inertia;
    if (radius == 0) radius = FLOAT_TO_FIXED(1.0f);

    fixed_t threshold = FIXED_MUL(model->outlier_threshold, radius);
    return distance > threshold;
}

int8_t kmeans_update(kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized) return -1;
    
    // WAITING_LABEL state: frozen, reject updates
    if (model->state == STATE_WAITING_LABEL) return -1;

    // Add to ring buffer (for potential labeling later)
    buffer_add_sample(&model->buffer, point, model->feature_dim);

    // Find nearest cluster
    fixed_t distance;
    uint8_t cluster_id = find_nearest_cluster(model, point, &distance);
    model->last_distance = distance;

    // Check outlier (after 10 samples baseline)
    bool is_outlier = false;
    if (model->buffer.count >= 10) {
        is_outlier = kmeans_is_outlier(model, point);
    }

    // State transitions
    if (is_outlier) {
        model->alarm_active = true;
        model->normal_streak = 0;
        model->alarm_sample_count++;
        
        if (model->state == STATE_NORMAL) {
            model->state = STATE_ALARM;
        }
        
        // In ALARM state, check if should transition to WAITING_LABEL
        if (model->state == STATE_ALARM && !model->motor_running) {
            model->state = STATE_WAITING_LABEL;
            model->waiting_label = true;
            model->buffer.frozen = true;
        }
        
        return -1;  // Outlier, don't assign cluster
    } else {
        model->normal_streak++;
        
        // Auto-clear alarm if back to normal for ALARM_CLEAR_SAMPLES
        if (model->state == STATE_ALARM && model->normal_streak >= ALARM_CLEAR_SAMPLES) {
            model->state = STATE_NORMAL;
            model->alarm_active = false;
            model->alarm_sample_count = 0;
        }
    }

    // Update centroid (only in NORMAL or ALARM state, and only for non-outliers)
    cluster_t* cluster = &model->clusters[cluster_id];
    
    float decay = 1.0f + 0.01f * cluster->count;
    float alpha_f = FIXED_TO_FLOAT(model->learning_rate) / decay;
    fixed_t alpha = FLOAT_TO_FIXED(alpha_f);

    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - cluster->centroid[i];
        cluster->centroid[i] += FIXED_MUL(alpha, diff);
    }
    
    cluster->inertia += FIXED_MUL(alpha, distance - cluster->inertia);
    cluster->count++;
    model->total_points++;

    return cluster_id;
}

uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point) {
    if (!model->initialized || model->k == 0) return 0;
    
    fixed_t distance;
    return find_nearest_cluster(model, point, &distance);
}

void kmeans_update_motor_status(kmeans_model_t* model, fixed_t rms, fixed_t current) {
    if (!model->initialized) return;
    
    model->last_rms = rms;
    model->last_current = current;
    
    // Detect idle: low vibration AND low current
    bool is_idle = (rms < IDLE_RMS_THRESHOLD);
    if (current > 0) {
        is_idle = is_idle && (current < IDLE_CURRENT_THRESHOLD);
    }
    
    if (is_idle) {
        model->idle_count++;
        if (model->idle_count >= IDLE_CONSECUTIVE_SAMPLES) {
            model->motor_running = false;
            
            // If in ALARM state and motor stopped -> WAITING_LABEL
            if (model->state == STATE_ALARM) {
                model->state = STATE_WAITING_LABEL;
                model->waiting_label = true;
                model->buffer.frozen = true;
            }
        }
    } else {
        model->idle_count = 0;
        model->motor_running = true;
        
        // If was WAITING_LABEL and motor restarts -> back to ALARM
        if (model->state == STATE_WAITING_LABEL && model->alarm_active) {
            model->state = STATE_ALARM;
            model->waiting_label = false;
            model->buffer.frozen = false;
        }
    }
}

void kmeans_request_label(kmeans_model_t* model) {
    if (!model->initialized) return;
    if (model->state == STATE_NORMAL && !model->alarm_active) return;
    
    // Manual button press: freeze for labeling
    model->state = STATE_WAITING_LABEL;
    model->waiting_label = true;
    model->buffer.frozen = true;
}

void kmeans_discard(kmeans_model_t* model) {
    if (model->state != STATE_WAITING_LABEL) return;

    model->state = STATE_NORMAL;
    model->alarm_active = false;
    model->waiting_label = false;
    model->alarm_sample_count = 0;
    model->normal_streak = 0;
    
    model->buffer.frozen = false;
    model->buffer.head = 0;
    model->buffer.count = 0;
}

bool kmeans_add_cluster(kmeans_model_t* model, const char* label) {
    if (!model->initialized) return false;
    if (model->k >= MAX_CLUSTERS) return false;
    if (!label || strlen(label) == 0) return false;
    if (model->state != STATE_WAITING_LABEL) return false;
    if (model->buffer.count == 0) return false;

    // Check duplicate label
    for (uint8_t i = 0; i < model->k; i++) {
        if (strcmp(model->clusters[i].label, label) == 0) return false;
    }

    cluster_t* new_cluster = &model->clusters[model->k];

    // NEW: Average ALL buffered samples (not just last one)
    memset(new_cluster->centroid, 0, model->feature_dim * sizeof(fixed_t));
    
    for (uint16_t i = 0; i < model->buffer.count; i++) {
        for (uint8_t d = 0; d < model->feature_dim; d++) {
            // Accumulate then divide to avoid overflow
            new_cluster->centroid[d] += model->buffer.samples[i][d] / (fixed_t)model->buffer.count;
        }
    }

    strncpy(new_cluster->label, label, MAX_LABEL_LENGTH - 1);
    new_cluster->label[MAX_LABEL_LENGTH - 1] = '\0';
    new_cluster->active = true;
    new_cluster->count = model->buffer.count;  // Start with buffer size
    new_cluster->inertia = FLOAT_TO_FIXED(1.0f);

    model->k++;

    // Clear alarm state
    model->state = STATE_NORMAL;
    model->alarm_active = false;
    model->waiting_label = false;
    model->alarm_sample_count = 0;
    model->normal_streak = 0;
    
    model->buffer.frozen = false;
    model->buffer.head = 0;
    model->buffer.count = 0;

    return true;
}

bool kmeans_assign_existing(kmeans_model_t* model, uint8_t cluster_id) {
    if (!model->initialized) return false;
    if (model->state != STATE_WAITING_LABEL) return false;
    if (cluster_id >= model->k) return false;
    if (model->buffer.count == 0) return false;

    cluster_t* cluster = &model->clusters[cluster_id];

    // Train existing cluster with ALL buffered samples via EMA
    for (uint16_t i = 0; i < model->buffer.count; i++) {
        fixed_t* sample = model->buffer.samples[i];
        
        // EMA update with decay
        float decay = 1.0f + 0.01f * cluster->count;
        fixed_t alpha = FLOAT_TO_FIXED(FIXED_TO_FLOAT(model->learning_rate) / decay);
        
        for (uint8_t d = 0; d < model->feature_dim; d++) {
            fixed_t diff = sample[d] - cluster->centroid[d];
            cluster->centroid[d] += FIXED_MUL(alpha, diff);
        }
        cluster->count++;
    }

    // Clear alarm state (same as add_cluster)
    model->state = STATE_NORMAL;
    model->alarm_active = false;
    model->waiting_label = false;
    model->alarm_sample_count = 0;
    model->normal_streak = 0;
    
    model->buffer.frozen = false;
    model->buffer.head = 0;
    model->buffer.count = 0;

    return true;
}

system_state_t kmeans_get_state(const kmeans_model_t* model) {
    return model->state;
}

bool kmeans_is_alarm_active(const kmeans_model_t* model) {
    return model->alarm_active;
}

bool kmeans_is_waiting_label(const kmeans_model_t* model) {
    return model->waiting_label;
}

bool kmeans_is_motor_running(const kmeans_model_t* model) {
    return model->motor_running;
}

uint16_t kmeans_get_buffer_size(const kmeans_model_t* model) {
    return model->buffer.frozen ? model->buffer.count : 0;
}

bool kmeans_get_centroid(const kmeans_model_t* model, uint8_t cluster_id, fixed_t* centroid) {
    if (!model->initialized || cluster_id >= model->k) return false;
    memcpy(centroid, model->clusters[cluster_id].centroid, model->feature_dim * sizeof(fixed_t));
    return true;
}

bool kmeans_get_label(const kmeans_model_t* model, uint8_t cluster_id, char* label) {
    if (!model->initialized || cluster_id >= model->k) return false;
    strncpy(label, model->clusters[cluster_id].label, MAX_LABEL_LENGTH);
    return true;
}

fixed_t kmeans_inertia(const kmeans_model_t* model) {
    if (!model->initialized) return 0;
    fixed_t total = 0;
    for (uint8_t i = 0; i < model->k; i++) {
        if (model->clusters[i].active) total += model->clusters[i].inertia;
    }
    return total;
}

void kmeans_reset(kmeans_model_t* model) {
    if (!model->initialized) return;
    uint8_t feature_dim = model->feature_dim;
    fixed_t lr = model->learning_rate;
    kmeans_init(model, feature_dim, FIXED_TO_FLOAT(lr));
}

bool kmeans_correct(kmeans_model_t* model, const fixed_t* point, uint8_t old_cluster, uint8_t new_cluster) {
    if (!model->initialized) return false;
    if (old_cluster >= model->k || new_cluster >= model->k) return false;
    if (old_cluster == new_cluster) return true;

    cluster_t* old = &model->clusters[old_cluster];
    fixed_t repel_rate = FLOAT_TO_FIXED(0.1f);
    for (uint8_t i = 0; i < model->feature_dim; i++) {
        fixed_t diff = point[i] - old->centroid[i];
        old->centroid[i] -= FIXED_MUL(repel_rate, diff);
    }
    if (old->count > 0) old->count--;

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