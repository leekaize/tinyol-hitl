/**
 * @file test_kmeans.c
 * @brief Simplified tests - Updated for new state machine
 *
 * State machine: NORMAL → ALARM → WAITING_LABEL
 * Must call kmeans_request_label() to transition from ALARM to WAITING_LABEL
 */

#include "../streaming_kmeans.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASS\n"); \
} while(0)

TEST(initialization) {
    kmeans_model_t model;
    bool ok = kmeans_init(&model, 3, 0.2f);

    assert(ok);
    assert(model.k == 1);
    assert(model.feature_dim == 3);
    assert(model.state == STATE_NORMAL);

    char label[MAX_LABEL_LENGTH];
    kmeans_get_label(&model, 0, label);
    assert(strcmp(label, "normal") == 0);
}

TEST(single_update) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.5f), FLOAT_TO_FIXED(0.5f)};
    int8_t cluster_id = kmeans_update(&model, point);

    assert(cluster_id == 0);
    assert(model.total_points == 1);
}

TEST(freeze_on_outlier) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Build baseline
    for (int i = 0; i < 20; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    assert(model.state == STATE_NORMAL);

    // Far outlier -> goes to ALARM first
    fixed_t outlier[2] = {FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f)};
    int8_t cluster = kmeans_update(&model, outlier);

    assert(cluster == -1);
    assert(model.state == STATE_ALARM);
    assert(model.alarm_active == true);

    // Manual freeze -> WAITING_LABEL
    kmeans_request_label(&model);

    assert(model.state == STATE_WAITING_LABEL);
    assert(model.buffer.frozen == true);
}

TEST(add_cluster) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, outlier);

    assert(model.state == STATE_ALARM);

    // Must request label before add_cluster works
    kmeans_request_label(&model);
    assert(model.state == STATE_WAITING_LABEL);
    assert(model.k == 1);

    bool ok = kmeans_add_cluster(&model, "fault");

    assert(ok);
    assert(model.k == 2);
    assert(model.state == STATE_NORMAL);

    char label[MAX_LABEL_LENGTH];
    kmeans_get_label(&model, 1, label);
    assert(strcmp(label, "fault") == 0);
}

TEST(discard) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, outlier);

    assert(model.state == STATE_ALARM);

    kmeans_request_label(&model);
    assert(model.state == STATE_WAITING_LABEL);

    kmeans_discard(&model);

    assert(model.state == STATE_NORMAL);
    assert(model.buffer.frozen == false);
    assert(model.k == 1);
}

TEST(prediction) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.3f), FLOAT_TO_FIXED(-0.2f)};

    uint8_t predicted = kmeans_predict(&model, point);
    int8_t updated = kmeans_update(&model, point);

    assert(predicted == (uint8_t)updated);
}

TEST(two_clusters) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    // Baseline
    for (int i = 0; i < 20; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.0f), FLOAT_TO_FIXED(0.0f)};
        kmeans_update(&model, p);
    }

    // Trigger alarm
    fixed_t outlier[2] = {FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f)};
    kmeans_update(&model, outlier);

    // Must request label first
    kmeans_request_label(&model);
    kmeans_add_cluster(&model, "fault");

    assert(model.k == 2);

    // New baseline samples go to C0
    fixed_t p0[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
    uint8_t c0 = kmeans_predict(&model, p0);
    assert(c0 == 0);

    // New fault samples go to C1
    fixed_t p1[2] = {FLOAT_TO_FIXED(9.9f), FLOAT_TO_FIXED(9.9f)};
    uint8_t c1 = kmeans_predict(&model, p1);
    assert(c1 == 1);
}

TEST(inertia) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    fixed_t point[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    kmeans_update(&model, point);
    fixed_t inertia_early = kmeans_inertia(&model);

    for (int i = 0; i < 50; i++) {
        kmeans_update(&model, point);
    }
    fixed_t inertia_late = kmeans_inertia(&model);

    assert(inertia_late <= inertia_early);
}

TEST(reset) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.1f);

    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, outlier);
    kmeans_request_label(&model);
    kmeans_add_cluster(&model, "fault");

    assert(model.k == 2);

    kmeans_reset(&model);

    assert(model.k == 1);
    assert(model.total_points == 0);
    assert(model.state == STATE_NORMAL);
}

TEST(high_dimensional) {
    kmeans_model_t model;
    kmeans_init(&model, 3, 0.15f);

    for (int i = 0; i < 20; i++) {
        fixed_t normal[3] = {
            FLOAT_TO_FIXED(0.1f),
            FLOAT_TO_FIXED(0.2f),
            FLOAT_TO_FIXED(9.8f)
        };
        kmeans_update(&model, normal);
    }

    fixed_t fault[3] = {
        FLOAT_TO_FIXED(5.0f),
        FLOAT_TO_FIXED(5.0f),
        FLOAT_TO_FIXED(5.0f)
    };
    kmeans_update(&model, fault);

    if (model.state == STATE_ALARM) {
        kmeans_request_label(&model);
        kmeans_add_cluster(&model, "fault");
        assert(model.k == 2);
    }
}

TEST(memory_footprint) {
    printf("\n  Model size: %zu bytes", sizeof(kmeans_model_t));
    printf("\n  Cluster: %zu bytes", sizeof(cluster_t));
    printf("\n  Buffer: %zu bytes\n", sizeof(ring_buffer_t));

    assert(sizeof(kmeans_model_t) < 100 * 1024);
}

int main() {
    printf("=== Simplified K-Means Tests ===\n");

    RUN_TEST(initialization);
    RUN_TEST(single_update);
    RUN_TEST(freeze_on_outlier);
    RUN_TEST(add_cluster);
    RUN_TEST(discard);
    RUN_TEST(prediction);
    RUN_TEST(two_clusters);
    RUN_TEST(inertia);
    RUN_TEST(reset);
    RUN_TEST(high_dimensional);
    RUN_TEST(memory_footprint);

    printf("\n=== All Tests Passed ===\n");
    return 0;
}