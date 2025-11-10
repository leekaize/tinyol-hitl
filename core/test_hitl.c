/**
 * @file test_hitl.c
 * @brief Tests for human-in-the-loop correction
 */

#include <stdio.h>
#include <math.h>
#include "streaming_kmeans.h"

#define TEST(name) printf("Running %s... ", #name); fflush(stdout);
#define PASS() printf("PASS\n")
#define FAIL(msg) do { printf("FAIL: %s\n", msg); return 1; } while(0)

int test_correction_basic() {
    TEST(correction_basic);

    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.3f);

    // Train two clusters
    fixed_t p1[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t p2[2] = {FLOAT_TO_FIXED(-1.0f), FLOAT_TO_FIXED(-1.0f)};

    for (int i = 0; i < 10; i++) {
        kmeans_update(&model, p1);
        kmeans_update(&model, p2);
    }

    // Point near cluster 0, but human says it's cluster 1
    fixed_t correction_point[2] = {FLOAT_TO_FIXED(0.9f), FLOAT_TO_FIXED(0.9f)};
    uint8_t assigned = kmeans_predict(&model, correction_point);

    // Apply correction
    if (!kmeans_correct(&model, correction_point, assigned, 1)) {
        FAIL("correction failed");
    }

    // Verify cluster 1 centroid moved toward correction_point
    fixed_t new_centroid[2];
    kmeans_get_centroid(&model, 1, new_centroid);
    float x = FIXED_TO_FLOAT(new_centroid[0]);

    if (x < -0.9f || x > -0.5f) FAIL("centroid didn't move");

    PASS();
    return 0;
}

int test_correction_noop() {
    TEST(correction_noop);

    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.3f);

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
    kmeans_init(&model, 2, 2, 0.3f);

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
    kmeans_init(&model, 2, 2, 0.3f);

    fixed_t p1[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};

    // Add points and track which cluster they go to
    uint8_t assigned_cluster = 0;
    for (int i = 0; i < 5; i++) {
        assigned_cluster = kmeans_update(&model, p1);
    }

    uint32_t count_before = model.clusters[assigned_cluster].count;
    uint32_t other_cluster = (assigned_cluster == 0) ? 1 : 0;
    uint32_t other_count_before = model.clusters[other_cluster].count;

    // Correct from assigned cluster to other cluster
    kmeans_correct(&model, p1, assigned_cluster, other_cluster);

    if (model.clusters[assigned_cluster].count != count_before - 1) FAIL("old cluster count wrong");
    if (model.clusters[other_cluster].count != other_count_before + 1) FAIL("new cluster count wrong");

    PASS();
    return 0;
}

int test_correction_multiple() {
    TEST(correction_multiple);

    kmeans_model_t model;
    kmeans_init(&model, 3, 2, 0.2f);

    // Create three clusters
    fixed_t p1[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(0.0f)};
    fixed_t p2[2] = {FLOAT_TO_FIXED(0.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t p3[2] = {FLOAT_TO_FIXED(-1.0f), FLOAT_TO_FIXED(0.0f)};

    for (int i = 0; i < 10; i++) {
        kmeans_update(&model, p1);
        kmeans_update(&model, p2);
        kmeans_update(&model, p3);
    }

    // Apply multiple corrections
    kmeans_correct(&model, p1, 0, 2);
    kmeans_correct(&model, p1, 0, 2);
    kmeans_correct(&model, p2, 1, 2);

    // Cluster 2 should have 3 corrections
    if (model.clusters[2].count < 3) FAIL("corrections not accumulated");

    PASS();
    return 0;
}

int main() {
    printf("=== HITL Correction Tests ===\n");

    if (test_correction_basic()) return 1;
    if (test_correction_noop()) return 1;
    if (test_correction_invalid()) return 1;
    if (test_correction_count_tracking()) return 1;
    if (test_correction_multiple()) return 1;

    printf("\n=== All HITL Tests Passed (5/5) ===\n");
    return 0;
}