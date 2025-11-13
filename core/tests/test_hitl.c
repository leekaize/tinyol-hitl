/**
 * @file test_hitl.c
 * @brief Tests for human-in-the-loop label corrections
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../streaming_kmeans.h"

#define TEST(name) printf("Running %s... ", #name); fflush(stdout);
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_correction_basic() {
    TEST(correction_basic);

    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    // Add second cluster
    fixed_t fault[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
    kmeans_add_cluster(&model, fault, "fault_a");

    // Train both clusters
    for (int i = 0; i < 10; i++) {
        fixed_t p0[2] = {FLOAT_TO_FIXED(0.1f), FLOAT_TO_FIXED(0.1f)};
        fixed_t p1[2] = {FLOAT_TO_FIXED(1.9f), FLOAT_TO_FIXED(1.9f)};
        kmeans_update(&model, p0);
        kmeans_update(&model, p1);
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

    if (x > 1.8f || x < 1.0f) FAIL("centroid didn't move enough");

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

    // Add second cluster
    fixed_t seed[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
    kmeans_add_cluster(&model, seed, "fault_a");

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
    if (model.clusters[1].count != 2) FAIL("new cluster count wrong");

    PASS();
    return 0;
}

int test_label_retrieval() {
    TEST(label_retrieval);

    kmeans_model_t model;
    kmeans_init(&model, 3, 0.2f);

    // Add clusters with labels
    fixed_t p1[3] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t p2[3] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};

    kmeans_add_cluster(&model, p1, "ball_fault");
    kmeans_add_cluster(&model, p2, "inner_race_fault");

    // Verify labels
    char label0[MAX_LABEL_LENGTH];
    char label1[MAX_LABEL_LENGTH];
    char label2[MAX_LABEL_LENGTH];

    kmeans_get_label(&model, 0, label0);
    kmeans_get_label(&model, 1, label1);
    kmeans_get_label(&model, 2, label2);

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

    // Add fault clusters
    fixed_t ball[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t inner[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};

    kmeans_add_cluster(&model, ball, "ball_fault");
    kmeans_add_cluster(&model, inner, "inner_race_fault");

    // Sample assigned to ball but is actually inner race
    fixed_t misclassified[2] = {FLOAT_TO_FIXED(1.1f), FLOAT_TO_FIXED(1.1f)};
    uint8_t assigned = kmeans_predict(&model, misclassified);

    if (assigned != 1) FAIL("should predict ball_fault");

    // Operator corrects: this is inner race
    kmeans_correct(&model, misclassified, 1, 2);

    // Verify inner race centroid moved
    fixed_t c_inner[2];
    kmeans_get_centroid(&model, 2, c_inner);
    float x = FIXED_TO_FLOAT(c_inner[0]);

    if (x > 2.0f) FAIL("centroid didn't attract");

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

    if (failures == 0) {
        printf("\n=== All HITL Tests Passed ===\n");
    } else {
        printf("\n=== %d HITL Tests Failed ===\n", failures);
    }

    return failures;
}