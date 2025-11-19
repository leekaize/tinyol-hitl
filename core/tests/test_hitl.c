/**
 * @file test_hitl.c
 * @brief Simplified HITL tests
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../streaming_kmeans.h"

#define TEST(name) printf("Running %s... ", #name); fflush(stdout);
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

void setup_frozen_state(kmeans_model_t* model) {
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(model, p);
    }

    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(model, outlier);
}

int test_correction_basic() {
    TEST(correction_basic);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    setup_frozen_state(&model);
    kmeans_add_cluster(&model, "fault");

    // Train clusters
    for (int i = 0; i < 10; i++) {
        fixed_t p0[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        fixed_t p1[2] = {FLOAT_TO_FIXED(4.9f), FLOAT_TO_FIXED(4.9f)};
        kmeans_update(&model, p0);

        int8_t c = kmeans_update(&model, p1);
        if (c == -1 && model.state == STATE_FROZEN) {
            kmeans_discard(&model);
        }
    }

    // Point near C0, operator corrects to C1
    fixed_t correction[2] = {FLOAT_TO_FIXED(0.2f), FLOAT_TO_FIXED(0.2f)};
    uint8_t assigned = kmeans_predict(&model, correction);

    if (assigned != 0) FAIL("should predict C0");

    kmeans_correct(&model, correction, 0, 1);

    // Verify C1 moved toward correction
    fixed_t c1[2];
    kmeans_get_centroid(&model, 1, c1);
    float x = FIXED_TO_FLOAT(c1[0]);

    if (x < 3.5f || x > 5.0f) FAIL("C1 should be between 3.5 and 5.0");

    PASS();
    return 0;
}

int test_correction_noop() {
    TEST(correction_noop);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    fixed_t point[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    kmeans_update(&model, point);

    fixed_t before[2];
    kmeans_get_centroid(&model, 0, before);

    kmeans_correct(&model, point, 0, 0);

    fixed_t after[2];
    kmeans_get_centroid(&model, 0, after);

    if (before[0] != after[0]) FAIL("centroid changed on no-op");

    PASS();
    return 0;
}

int test_correction_count() {
    TEST(correction_count);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    setup_frozen_state(&model);
    kmeans_add_cluster(&model, "fault");

    fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
    for (int i = 0; i < 5; i++) {
        kmeans_update(&model, p);
    }

    uint32_t count_before = model.clusters[0].count;

    kmeans_correct(&model, p, 0, 1);

    uint32_t count_after = model.clusters[0].count;

    if (count_after != count_before - 1) FAIL("count not decremented");

    PASS();
    return 0;
}

int test_label_retrieval() {
    TEST(label_retrieval);

    kmeans_model_t model;
    kmeans_init(&model, 3, 0.2f);

    // First fault
    for (int i = 0; i < 15; i++) {
        fixed_t p[3] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }
    fixed_t outlier1[3] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, outlier1);
    kmeans_add_cluster(&model, "ball_fault");

    // Second fault (far from first)
    for (int i = 0; i < 15; i++) {
        fixed_t p[3] = {FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f)};
        int8_t c = kmeans_update(&model, p);
        if (c == -1 && model.state == STATE_FROZEN) {
            kmeans_discard(&model);
        }
    }
    fixed_t outlier2[3] = {FLOAT_TO_FIXED(20.0f), FLOAT_TO_FIXED(20.0f), FLOAT_TO_FIXED(20.0f)};
    kmeans_update(&model, outlier2);

    if (model.state != STATE_FROZEN) FAIL("second outlier didn't freeze");

    kmeans_add_cluster(&model, "inner_race");

    // Verify labels
    char label0[MAX_LABEL_LENGTH];
    char label1[MAX_LABEL_LENGTH];
    char label2[MAX_LABEL_LENGTH];

    kmeans_get_label(&model, 0, label0);
    kmeans_get_label(&model, 1, label1);
    kmeans_get_label(&model, 2, label2);

    if (strcmp(label0, "normal") != 0) FAIL("C0 wrong");
    if (strcmp(label1, "ball_fault") != 0) FAIL("C1 wrong");
    if (strcmp(label2, "inner_race") != 0) FAIL("C2 wrong");

    PASS();
    return 0;
}

int test_correction_with_labels() {
    TEST(correction_with_labels);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Baseline
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
        kmeans_update(&model, p);
    }

    // Ball fault
    fixed_t ball[2] = {FLOAT_TO_FIXED(8.0f), FLOAT_TO_FIXED(8.0f)};
    kmeans_update(&model, ball);
    kmeans_add_cluster(&model, "ball_fault");

    // Inner race (far from ball)
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(15.0f), FLOAT_TO_FIXED(15.0f)};
        int8_t c = kmeans_update(&model, p);
        if (c == -1 && model.state == STATE_FROZEN) {
            kmeans_discard(&model);
        }
    }
    fixed_t inner[2] = {FLOAT_TO_FIXED(25.0f), FLOAT_TO_FIXED(25.0f)};
    kmeans_update(&model, inner);
    kmeans_add_cluster(&model, "inner_race");

    // Point closer to ball but operator says inner
    fixed_t misclass[2] = {FLOAT_TO_FIXED(8.1f), FLOAT_TO_FIXED(8.1f)};
    uint8_t assigned = kmeans_predict(&model, misclass);

    if (assigned != 1) FAIL("should predict ball_fault");

    kmeans_correct(&model, misclass, 1, 2);

    fixed_t c_inner[2];
    kmeans_get_centroid(&model, 2, c_inner);
    float x = FIXED_TO_FLOAT(c_inner[0]);

    if (x > 25.0f) FAIL("centroid didn't move");

    PASS();
    return 0;
}

int test_freeze_workflow() {
    TEST(freeze_workflow);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    for (int i = 0; i < 20; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    fixed_t outlier[2] = {FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f)};
    int8_t result = kmeans_update(&model, outlier);

    if (result != -1) FAIL("should return -1");
    if (model.state != STATE_FROZEN) FAIL("should be frozen");

    kmeans_add_cluster(&model, "fault");

    if (model.state != STATE_NORMAL) FAIL("should resume");
    if (model.k != 2) FAIL("k should be 2");

    PASS();
    return 0;
}

int test_discard_workflow() {
    TEST(discard_workflow);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    setup_frozen_state(&model);

    if (model.state != STATE_FROZEN) FAIL("should be frozen");

    kmeans_discard(&model);

    if (model.state != STATE_NORMAL) FAIL("should resume");
    if (model.k != 1) FAIL("k should be 1");

    PASS();
    return 0;
}

int test_idle_detection() {
    TEST(idle_detection);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    setup_frozen_state(&model);

    // Motor running
    for (int i = 0; i < 5; i++) {
        kmeans_update_idle(&model, FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(1.5f));
    }

    if (model.state != STATE_FROZEN) FAIL("should stay frozen");

    // Motor stops
    for (int i = 0; i < 12; i++) {
        kmeans_update_idle(&model, FLOAT_TO_FIXED(0.2f), FLOAT_TO_FIXED(0.05f));
    }

    if (model.state != STATE_FROZEN_IDLE) FAIL("should be idle");
    if (!kmeans_is_idle(&model)) FAIL("should report idle");

    kmeans_add_cluster(&model, "fault");

    if (model.state != STATE_NORMAL) FAIL("should resume");

    PASS();
    return 0;
}

int test_idle_persistence() {
    TEST(idle_persistence);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    setup_frozen_state(&model);

    // Go idle
    for (int i = 0; i < 12; i++) {
        kmeans_update_idle(&model, FLOAT_TO_FIXED(0.1f), 0);
    }

    if (model.state != STATE_FROZEN_IDLE) FAIL("should be idle");

    // Stay idle for long time
    for (int i = 0; i < 100; i++) {
        kmeans_update_idle(&model, FLOAT_TO_FIXED(0.1f), 0);
    }

    if (model.state != STATE_FROZEN_IDLE) FAIL("should persist");

    // Motor restarts
    for (int i = 0; i < 5; i++) {
        kmeans_update_idle(&model, FLOAT_TO_FIXED(5.0f), 0);
    }

    if (model.state != STATE_FROZEN) FAIL("should return to frozen");

    PASS();
    return 0;
}

int main() {
    int failures = 0;

    failures += test_correction_basic();
    failures += test_correction_noop();
    failures += test_correction_count();
    failures += test_label_retrieval();
    failures += test_correction_with_labels();
    failures += test_freeze_workflow();
    failures += test_discard_workflow();
    failures += test_idle_detection();
    failures += test_idle_persistence();

    if (failures == 0) {
        printf("\n=== All HITL Tests Passed ===\n");
    } else {
        printf("\n=== %d Tests Failed ===\n", failures);
    }

    return failures;
}