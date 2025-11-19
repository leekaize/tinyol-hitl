/**
 * @file test_hitl.c
 * @brief Tests for human-in-the-loop label corrections with freeze workflow
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../streaming_kmeans.h"

#define TEST(name) printf("Running %s... ", #name); fflush(stdout);
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

// Helper: build baseline cluster and trigger alarm
void setup_frozen_state(kmeans_model_t* model) {
    // Build baseline
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(model, p);
    }

    // Trigger alarm
    fixed_t outlier[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(model, outlier);
}

int test_correction_basic() {
    TEST(correction_basic);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    setup_frozen_state(&model);

    // Label the frozen outlier
    if (!kmeans_add_cluster(&model, "fault_a")) {
        FAIL("failed to add cluster");
    }

    if (model.k != 2) FAIL("k should be 2");

    // Train both clusters
    for (int i = 0; i < 10; i++) {
        fixed_t p0[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        fixed_t p1[2] = {FLOAT_TO_FIXED(4.9f), FLOAT_TO_FIXED(4.9f)};
        kmeans_update(&model, p0);

        // Check if update triggers alarm
        int8_t cluster = kmeans_update(&model, p1);
        if (cluster == -1 && model.state == STATE_FROZEN) {
            kmeans_discard(&model);  // False alarm, discard
        }
    }

    // Point near cluster 0, but operator says it's cluster 1
    fixed_t correction[2] = {FLOAT_TO_FIXED(0.2f), FLOAT_TO_FIXED(0.2f)};
    uint8_t assigned = kmeans_predict(&model, correction);

    if (assigned != 0) FAIL("point should be near cluster 0");

    // Apply correction
    if (!kmeans_correct(&model, correction, 0, 1)) {
        FAIL("correction failed");
    }

    // Verify cluster 1 moved toward correction point
    fixed_t c1[2];
    kmeans_get_centroid(&model, 1, c1);
    float x = FIXED_TO_FLOAT(c1[0]);

    if (x > 4.8f || x < 1.0f) FAIL("centroid didn't move enough");

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

    // Correct to same cluster (no-op)
    kmeans_correct(&model, point, 0, 0);

    fixed_t after[2];
    kmeans_get_centroid(&model, 0, after);

    if (before[0] != after[0]) FAIL("centroid changed on no-op");

    PASS();
    return 0;
}

int test_correction_invalid() {
    TEST(correction_invalid);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    fixed_t point[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};

    // Invalid cluster IDs
    if (kmeans_correct(&model, point, 0, 5)) FAIL("accepted invalid new_cluster");
    if (kmeans_correct(&model, point, 5, 0)) FAIL("accepted invalid old_cluster");

    PASS();
    return 0;
}

int test_correction_count_tracking() {
    TEST(correction_count_tracking);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    setup_frozen_state(&model);
    kmeans_add_cluster(&model, "fault_a");

    // Add points to cluster 0
    fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
    for (int i = 0; i < 5; i++) {
        kmeans_update(&model, p);
    }

    uint32_t count_before = model.clusters[0].count;

    // Correct one point from cluster 0 to cluster 1
    kmeans_correct(&model, p, 0, 1);

    uint32_t count_after = model.clusters[0].count;

    if (count_after != count_before - 1) FAIL("count not decremented");
    if (model.clusters[1].count < 1) FAIL("new cluster count wrong");

    PASS();
    return 0;
}

int test_label_retrieval() {
    TEST(label_retrieval);

    kmeans_model_t model;
    kmeans_init(&model, 3, 0.2f);

    // Build and label first fault
    setup_frozen_state(&model);
    kmeans_add_cluster(&model, "ball_fault");

    // Build and label second fault
    for (int i = 0; i < 15; i++) {
        fixed_t p[3] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
        kmeans_update(&model, p);
    }
    fixed_t outlier2[3] = {FLOAT_TO_FIXED(8.0f), FLOAT_TO_FIXED(8.0f), FLOAT_TO_FIXED(8.0f)};
    kmeans_update(&model, outlier2);
    kmeans_add_cluster(&model, "inner_race_fault");

    // Verify labels
    char label0[MAX_LABEL_LENGTH];
    char label1[MAX_LABEL_LENGTH];
    char label2[MAX_LABEL_LENGTH];

    kmeans_get_label(&model, 0, label0);
    kmeans_get_label(&model, 1, label1);
    kmeans_get_label(&model, 2, label2);

    printf("\n  Checking labels:\n");
    printf("  C0: '%s' (expected: 'normal')\n", label0);
    printf("  C1: '%s' (expected: 'ball_fault')\n", label1);
    printf("  C2: '%s' (expected: 'inner_race_fault')\n", label2);
    printf("  K=%u\n", model.k);

    if (strcmp(label0, "normal") != 0) FAIL("cluster 0 label wrong");
    if (strcmp(label1, "ball_fault") != 0) FAIL("cluster 1 label wrong");
    if (strcmp(label2, "inner_race_fault") != 0) FAIL("cluster 2 label wrong");

    PASS();
    return 0;
}

int test_correction_with_labels() {
    TEST(correction_with_labels);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Build baseline
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
        kmeans_update(&model, p);
    }

    // Add ball fault cluster
    fixed_t ball[2] = {FLOAT_TO_FIXED(5.0f), FLOAT_TO_FIXED(5.0f)};
    kmeans_update(&model, ball);
    kmeans_add_cluster(&model, "ball_fault");

    // Add inner race cluster
    for (int i = 0; i < 15; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
        kmeans_update(&model, p);
    }
    fixed_t inner[2] = {FLOAT_TO_FIXED(8.0f), FLOAT_TO_FIXED(8.0f)};
    kmeans_update(&model, inner);
    kmeans_add_cluster(&model, "inner_race_fault");

    // Sample assigned to ball but is actually inner race
    fixed_t misclassified[2] = {FLOAT_TO_FIXED(5.1f), FLOAT_TO_FIXED(5.1f)};
    uint8_t assigned = kmeans_predict(&model, misclassified);

    if (assigned != 1) FAIL("should predict ball_fault");

    // Operator corrects: this is inner race
    kmeans_correct(&model, misclassified, 1, 2);

    // Verify inner race centroid moved
    fixed_t c_inner[2];
    kmeans_get_centroid(&model, 2, c_inner);
    float x = FIXED_TO_FLOAT(c_inner[0]);

    if (x > 8.0f) FAIL("centroid didn't attract");

    PASS();
    return 0;
}

int test_freeze_workflow() {
    TEST(freeze_workflow);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Build baseline
    for (int i = 0; i < 20; i++) {
        fixed_t p[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        kmeans_update(&model, p);
    }

    if (model.state != STATE_NORMAL) FAIL("should start in NORMAL");

    // Trigger alarm
    fixed_t outlier[2] = {FLOAT_TO_FIXED(10.0f), FLOAT_TO_FIXED(10.0f)};
    int8_t result = kmeans_update(&model, outlier);

    if (result != -1) FAIL("should return -1 when frozen");
    if (model.state != STATE_FROZEN) FAIL("should be FROZEN");
    if (!model.buffer.frozen) FAIL("buffer should be frozen");

    uint16_t buffer_size = kmeans_get_buffer_size(&model);
    if (buffer_size == 0) FAIL("buffer should have samples");

    // Try to update while frozen (should be rejected)
    fixed_t another[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    result = kmeans_update(&model, another);
    if (result != -1) FAIL("updates should be rejected when frozen");

    // Label the outlier
    if (!kmeans_add_cluster(&model, "new_fault")) {
        FAIL("labeling should succeed");
    }

    if (model.state != STATE_NORMAL) FAIL("should resume NORMAL after label");
    if (model.k != 2) FAIL("k should be 2");
    if (kmeans_get_buffer_size(&model) != 0) FAIL("buffer should be cleared");

    PASS();
    return 0;
}

int test_discard_workflow() {
    TEST(discard_workflow);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    setup_frozen_state(&model);

    if (model.state != STATE_FROZEN) FAIL("should be frozen");
    if (kmeans_get_buffer_size(&model) == 0) FAIL("buffer should have samples");

    // Operator discards (false alarm)
    kmeans_discard(&model);

    if (model.state != STATE_NORMAL) FAIL("should resume NORMAL");
    if (model.k != 1) FAIL("k should still be 1");
    if (kmeans_get_buffer_size(&model) != 0) FAIL("buffer should be cleared");

    PASS();
    return 0;
}

int test_idle_detection() {
    TEST(idle_detection);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Trigger alarm
    setup_frozen_state(&model);
    if (model.state != STATE_FROZEN) FAIL("should be frozen");

    // Simulate motor running (high RMS)
    for (int i = 0; i < 5; i++) {
        fixed_t rms = FLOAT_TO_FIXED(5.0f);
        fixed_t current = FLOAT_TO_FIXED(1.5f);
        kmeans_update_idle(&model, rms, current);
    }

    if (model.state != STATE_FROZEN) FAIL("should still be frozen while running");

    // Motor stops (low RMS, low current)
    for (int i = 0; i < 12; i++) {
        fixed_t rms = FLOAT_TO_FIXED(0.2f);  // Below threshold
        fixed_t current = FLOAT_TO_FIXED(0.05f);  // Below threshold
        kmeans_update_idle(&model, rms, current);
    }

    if (model.state != STATE_FROZEN_IDLE) FAIL("should transition to FROZEN_IDLE");
    if (!kmeans_is_idle(&model)) FAIL("should report as idle");

    // Operator can still label while idle
    if (!kmeans_add_cluster(&model, "fault_detected")) {
        FAIL("labeling should work in idle state");
    }

    if (model.state != STATE_NORMAL) FAIL("should resume after labeling");

    PASS();
    return 0;
}

int test_idle_persistence() {
    TEST(idle_persistence);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Alarm during operation
    setup_frozen_state(&model);

    // Motor goes idle
    for (int i = 0; i < 12; i++) {
        fixed_t rms = FLOAT_TO_FIXED(0.1f);
        kmeans_update_idle(&model, rms, 0);
    }

    if (model.state != STATE_FROZEN_IDLE) FAIL("should be frozen idle");

    // Hours pass (simulate with more updates)
    for (int i = 0; i < 100; i++) {
        fixed_t rms = FLOAT_TO_FIXED(0.1f);
        kmeans_update_idle(&model, rms, 0);
    }

    // Should still be frozen
    if (model.state != STATE_FROZEN_IDLE) FAIL("should persist in idle");

    // Motor restarts
    for (int i = 0; i < 5; i++) {
        fixed_t rms = FLOAT_TO_FIXED(5.0f);
        kmeans_update_idle(&model, rms, 0);
    }

    // Should return to FROZEN (not auto-resume)
    if (model.state != STATE_FROZEN) FAIL("should return to frozen, not normal");

    PASS();
    return 0;
}

int main() {
    int failures = 0;

    failures += test_correction_basic();
    failures += test_correction_noop();
    failures += test_correction_invalid();
    failures += test_correction_count_tracking();
    failures += test_label_retrieval();
    failures += test_correction_with_labels();
    failures += test_freeze_workflow();
    failures += test_discard_workflow();
    failures += test_idle_detection();
    failures += test_idle_persistence();

    if (failures == 0) {
        printf("\n=== All HITL Tests Passed ===\n");
    } else {
        printf("\n=== %d HITL Tests Failed ===\n", failures);
    }

    return failures;
}