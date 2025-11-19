/**
 * @file streaming_kmeans.h
 * @brief Simplified label-driven clustering
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

typedef enum {
    STATE_NORMAL,
    STATE_ALARM,
    STATE_FROZEN,
    STATE_FROZEN_IDLE
} system_state_t;

#define IDLE_RMS_THRESHOLD FLOAT_TO_FIXED(0.5f)
#define IDLE_CURRENT_THRESHOLD FLOAT_TO_FIXED(0.1f)
#define IDLE_CONSECUTIVE_SAMPLES 10

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
    
    system_state_t state;
    ring_buffer_t buffer;
    fixed_t outlier_threshold;
    fixed_t last_distance;
    
    uint8_t idle_count;
    fixed_t last_rms;
    fixed_t last_current;
} kmeans_model_t;

#ifdef __cplusplus
extern "C" {
#endif

bool kmeans_init(kmeans_model_t* model, uint8_t feature_dim, float learning_rate);
int8_t kmeans_update(kmeans_model_t* model, const fixed_t* point);
uint8_t kmeans_predict(const kmeans_model_t* model, const fixed_t* point);
bool kmeans_add_cluster(kmeans_model_t* model, const char* label);
bool kmeans_is_outlier(const kmeans_model_t* model, const fixed_t* point);
void kmeans_alarm(kmeans_model_t* model);
void kmeans_discard(kmeans_model_t* model);
system_state_t kmeans_get_state(const kmeans_model_t* model);
uint16_t kmeans_get_buffer_size(const kmeans_model_t* model);
bool kmeans_get_centroid(const kmeans_model_t* model, uint8_t cluster_id, fixed_t* centroid);
bool kmeans_get_label(const kmeans_model_t* model, uint8_t cluster_id, char* label);
fixed_t kmeans_inertia(const kmeans_model_t* model);
void kmeans_reset(kmeans_model_t* model);
bool kmeans_correct(kmeans_model_t* model, const fixed_t* point, uint8_t old_cluster, uint8_t new_cluster);
void kmeans_set_threshold(kmeans_model_t* model, float multiplier);
void kmeans_update_idle(kmeans_model_t* model, fixed_t rms, fixed_t current);
bool kmeans_is_idle(const kmeans_model_t* model);

#ifdef __cplusplus
}
#endif

#endif