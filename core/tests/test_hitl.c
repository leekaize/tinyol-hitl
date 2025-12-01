/**
 * @file test_hitl.c
 * @brief HITL tests - Updated for new state machine
 *
 * State machine: NORMAL → ALARM → WAITING_LABEL
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../streaming_kmeans.h"

#define TEST(name) printf("Running %s... ", #name); fflush(stdout);
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

// Helper: Create outlier and transition to WAITING_LABEL
void setup_waiting_label_state(kmeans_model_t* model) {
    // Build baseline
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(model, p);
    }

    // Outlier -> ALARM
    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(model, outlier);

    // Request label -> WAITING_LABEL
    kmeans_request_label(model);
}

int test_correction_basic() {
    TEST(correction_basic);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    setup_waiting_label_state(&model);
    kmeans_add_cluster(&model, "fault");

    // Train clusters
    for (int i = 0; i < 10; i++) {
        fixed_t p0[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        fixed_t p1[2] = {FLOAT_TO_FIXED(4.9f), FLOAT_TO_FIXED(4.9f)};
        kmeans_update(&model, p0);

        int8_t c = kmeans_update(&model, p1);
        if (c == -1 && model.state == STATE_ALARM) {
            kmeans_request_label(&model);
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

    setup_waiting_label_state(&model);
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
    setup_waiting_label_state(&model);
    kmeans_add_cluster(&model, "ball_fault");

    // Train first fault cluster
    for (int i = 0; i < 15; i++) {
        fixed_t p[3] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
        int8_t c = kmeans_update(&model, p);
        if (c == -1 && model.state == STATE_ALARM) {
            kmeans_request_label(&model);
            kmeans_discard(&model);
        }
    }

    // Second fault (far from first) - need new baseline first
    for (int i = 0; i < 15; i++) {
        fixed_t p[3] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    fixed_t outlier2[3] = {FLOAT_TO_FIXED(20.0f), FLOAT_TO_FIXED(20.0f), FLOAT_TO_FIXED(20.0f)};
    kmeans_update(&model, outlier2);

    if (model.state != STATE_ALARM) FAIL("second outlier didn't trigger alarm");

    kmeans_request_label(&model);
    if (model.state != STATE_WAITING_LABEL) FAIL("should be waiting label");

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
    kmeans_request_label(&model);
    kmeans_add_cluster(&model, "ball_fault");

    // Train ball cluster
    for (int i = 0; i < 10; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(8.0f), FLOAT_TO_FIXED(8.0f)};
        int8_t c = kmeans_update(&model, p);
        if (c == -1 && model.state == STATE_ALARM) {
            kmeans_request_label(&model);
            kmeans_discard(&model);
        }
    }

    // Inner race (far from ball)
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
        kmeans_update(&model, p);
    }

    fixed_t inner[2] = {FLOAT_TO_FIXED(25.0f), FLOAT_TO_FIXED(25.0f)};
    kmeans_update(&model, inner);
    kmeans_request_label(&model);
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
    if (model.state != STATE_ALARM) FAIL("should be in alarm");

    kmeans_request_label(&model);
    if (model.state != STATE_WAITING_LABEL) FAIL("should be waiting label");

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

    setup_waiting_label_state(&model);

    if (model.state != STATE_WAITING_LABEL) FAIL("should be waiting label");

    kmeans_discard(&model);

    if (model.state != STATE_NORMAL) FAIL("should resume");
    if (model.k != 1) FAIL("k should be 1");

    PASS();
    return 0;
}

int test_assign_existing() {
    TEST(assign_existing);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Build baseline cluster
    for (int i = 0; i < 20; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    // First anomaly → new label "fault_A"
    fixed_t outlier1[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, outlier1);

    if (model.state != STATE_ALARM) FAIL("should be in alarm");

    kmeans_request_label(&model);
    kmeans_add_cluster(&model, "fault_A");
    if (model.k != 2) FAIL("K should be 2");

    // Train fault_A cluster a bit
    for (int i = 0; i < 10; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(5.1f), FLOAT_TO_FIXED(5.1f)};
        int8_t c = kmeans_update(&model, p);
        if (c == -1 && model.state == STATE_ALARM) {
            kmeans_request_label(&model);
            kmeans_discard(&model);
        }
    }

    uint32_t count_before = model.clusters[1].count;

    // Second anomaly (similar to fault_A) → assign existing, not new K
    fixed_t outlier2[2] = {FLOAT_TO_FIXED(15.0f), FLOAT_TO_FIXED(15.0f)};
    kmeans_update(&model, outlier2);

    if (model.state != STATE_ALARM) FAIL("should be in alarm");

    kmeans_request_label(&model);
    if (model.state != STATE_WAITING_LABEL) FAIL("should be waiting label");

    // Assign to existing cluster 1 (fault_A)
    bool ok = kmeans_assign_existing(&model, 1);

    if (!ok) FAIL("assign_existing should succeed");
    if (model.k != 2) FAIL("K should still be 2");
    if (model.state != STATE_NORMAL) FAIL("should resume normal");
    if (model.clusters[1].count <= count_before) FAIL("count should increase");

    PASS();
    return 0;
}

int test_assign_existing_invalid() {
    TEST(assign_existing_invalid);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Try assign when not in waiting_label (should fail)
    bool ok = kmeans_assign_existing(&model, 0);
    if (ok) FAIL("should fail when not in waiting_label");

    // Try assign to invalid cluster
    setup_waiting_label_state(&model);
    ok = kmeans_assign_existing(&model, 99);
    if (ok) FAIL("should fail for invalid cluster_id");

    PASS();
    return 0;
}

int test_motor_status() {
    TEST(motor_status);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Initially motor is running
    if (!kmeans_is_motor_running(&model)) FAIL("motor should start as running");

    // Simulate motor running (high vibration)
    for (int i = 0; i < 5; i++) {
        kmeans_update_motor_status(&model, FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(1.5f));
    }
    if (!kmeans_is_motor_running(&model)) FAIL("motor should be running with high vibration");

    // Simulate motor stopping (low vibration for 10+ samples)
    for (int i = 0; i < 12; i++) {
        kmeans_update_motor_status(&model, FLOAT_TO_FIXED(0.2f), FLOAT_TO_FIXED(0.05f));
    }
    if (kmeans_is_motor_running(&model)) FAIL("motor should be stopped");

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
    failures += test_assign_existing();
    failures += test_assign_existing_invalid();
    failures += test_motor_status();

    if (failures == 0) {
        printf("\n=== All HITL Tests Passed ===\n");
    } else {
        printf("\n=== %d Tests Failed ===\n", failures);
    }

    return failures;
}