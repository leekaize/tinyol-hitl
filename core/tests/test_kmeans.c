/**
 * @file test_kmeans.c
 * @brief Tests for dynamic label-driven clustering
 *
 * Compile: gcc -o test_kmeans test_kmeans.c streaming_kmeans.c -lm
 * Run: ./test_kmeans
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

TEST(initialization_starts_k_equals_one) {
    kmeans_model_t model;
    bool ok = kmeans_init(&model, 3, 0.2f);

    assert(ok);
    assert(model.initialized);
    assert(model.k == 1);  // Starts with single cluster
    assert(model.feature_dim == 3);
    assert(model.total_points == 0);

    char label[MAX_LABEL_LENGTH];
    kmeans_get_label(&model, 0, label);
    assert(strcmp(label, "normal") == 0);
}

TEST(invalid_params) {
    kmeans_model_t model;

    // Too many features
    assert(!kmeans_init(&model, MAX_FEATURES + 1, 0.1f));

    // Zero features
    assert(!kmeans_init(&model, 0, 0.1f));
}

TEST(single_point_update_k_equals_one) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.5f), FLOAT_TO_FIXED(0.5f)};
    uint8_t cluster_id = kmeans_update(&model, point);

    assert(cluster_id == 0);  // Only one cluster exists
    assert(model.total_points == 1);
    assert(model.clusters[0].count == 1);
}

TEST(add_cluster_increases_k) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Add second cluster
    fixed_t fault_point[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
    bool ok = kmeans_add_cluster(&model, fault_point, "outer_race_fault");

    assert(ok);
    assert(model.k == 2);

    // Verify label
    char label[MAX_LABEL_LENGTH];
    kmeans_get_label(&model, 1, label);
    assert(strcmp(label, "outer_race_fault") == 0);

    // Verify centroid
    fixed_t centroid[2];
    kmeans_get_centroid(&model, 1, centroid);
    assert(centroid[0] == fault_point[0]);
    assert(centroid[1] == fault_point[1]);
}

TEST(add_cluster_rejects_duplicate_label) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    fixed_t point1[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t point2[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};

    // Add first cluster
    assert(kmeans_add_cluster(&model, point1, "ball_fault"));
    assert(model.k == 2);

    // Try to add same label again
    bool ok = kmeans_add_cluster(&model, point2, "ball_fault");
    assert(!ok);  // Should reject
    assert(model.k == 2);  // K unchanged
}

TEST(add_cluster_enforces_max_k) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    // Fill up to MAX_CLUSTERS
    for (int i = 1; i < MAX_CLUSTERS; i++) {
        fixed_t point[2] = {FLOAT_TO_FIXED(i), FLOAT_TO_FIXED(i)};
        char label[32];
        snprintf(label, sizeof(label), "fault_%d", i);
        assert(kmeans_add_cluster(&model, point, label));
    }

    assert(model.k == MAX_CLUSTERS);

    // Try to add one more
    fixed_t overflow[2] = {FLOAT_TO_FIXED(99.0f), FLOAT_TO_FIXED(99.0f)};
    bool ok = kmeans_add_cluster(&model, overflow, "overflow_fault");
    assert(!ok);  // Should reject
    assert(model.k == MAX_CLUSTERS);  // K unchanged
}

TEST(prediction_matches_update) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.3f), FLOAT_TO_FIXED(-0.2f)};

    uint8_t predicted = kmeans_predict(&model, point);
    uint8_t updated = kmeans_update(&model, point);

    assert(predicted == updated);
}

TEST(centroid_convergence_two_clusters) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.3f);

    // Add second cluster
    fixed_t seed[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
    kmeans_add_cluster(&model, seed, "fault_a");

    // Stream points from two Gaussians
    for (int i = 0; i < 50; i++) {
        // Cluster 0: around (0,0)
        float noise = 0.2f * (rand() % 100 - 50) / 50.0f;
        fixed_t p0[2] = {FLOAT_TO_FIXED(noise), FLOAT_TO_FIXED(noise)};
        kmeans_update(&model, p0);

        // Cluster 1: around (2,2)
        noise = 0.2f * (rand() % 100 - 50) / 50.0f;
        fixed_t p1[2] = {
            FLOAT_TO_FIXED(2.0f + noise),
            FLOAT_TO_FIXED(2.0f + noise)
        };
        kmeans_update(&model, p1);
    }

    // Check convergence
    assert(model.total_points == 100);
    assert(model.clusters[0].count > 0);
    assert(model.clusters[1].count > 0);

    // Verify centroids roughly correct
    fixed_t c0[2], c1[2];
    kmeans_get_centroid(&model, 0, c0);
    kmeans_get_centroid(&model, 1, c1);

    float c0_x = FIXED_TO_FLOAT(c0[0]);
    float c1_x = FIXED_TO_FLOAT(c1[0]);

    // Cluster 0 should be near origin
    assert(fabs(c0_x) < 0.5f);

    // Cluster 1 should be near (2,2)
    assert(fabs(c1_x - 2.0f) < 0.5f);
}

TEST(inertia_decreases_over_time) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.2f);

    fixed_t point[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};

    // First update
    kmeans_update(&model, point);
    fixed_t inertia_early = kmeans_inertia(&model);

    // Many more updates with same point
    for (int i = 0; i < 50; i++) {
        kmeans_update(&model, point);
    }
    fixed_t inertia_late = kmeans_inertia(&model);

    // Inertia should stabilize (decrease or stay constant)
    assert(inertia_late <= inertia_early);
}

TEST(reset_returns_to_k_equals_one) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 0.1f);

    // Add clusters
    fixed_t p1[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    fixed_t p2[2] = {FLOAT_TO_FIXED(2.0f), FLOAT_TO_FIXED(2.0f)};
    kmeans_add_cluster(&model, p1, "fault_a");
    kmeans_add_cluster(&model, p2, "fault_b");

    assert(model.k == 3);

    // Process some points
    kmeans_update(&model, p1);
    kmeans_update(&model, p2);
    assert(model.total_points == 2);

    // Reset
    kmeans_reset(&model);

    assert(model.k == 1);
    assert(model.total_points == 0);
    assert(model.initialized);

    char label[MAX_LABEL_LENGTH];
    kmeans_get_label(&model, 0, label);
    assert(strcmp(label, "normal") == 0);
}

TEST(high_dimensional_3d_accel) {
    kmeans_model_t model;
    kmeans_init(&model, 3, 0.15f);  // 3D accelerometer

    // Normal operation
    for (int i = 0; i < 20; i++) {
        fixed_t normal[3] = {
            FLOAT_TO_FIXED(0.1f + 0.05f * (rand() % 100) / 100.0f),
            FLOAT_TO_FIXED(0.2f + 0.05f * (rand() % 100) / 100.0f),
            FLOAT_TO_FIXED(9.8f + 0.2f * (rand() % 100) / 100.0f)
        };
        kmeans_update(&model, normal);
    }

    // Add fault cluster
    fixed_t fault[3] = {
        FLOAT_TO_FIXED(2.5f),
        FLOAT_TO_FIXED(1.8f),
        FLOAT_TO_FIXED(9.5f)
    };
    kmeans_add_cluster(&model, fault, "bearing_fault");

    // Fault samples
    for (int i = 0; i < 20; i++) {
        fixed_t f[3] = {
            FLOAT_TO_FIXED(2.5f + 0.3f * (rand() % 100) / 100.0f),
            FLOAT_TO_FIXED(1.8f + 0.3f * (rand() % 100) / 100.0f),
            FLOAT_TO_FIXED(9.5f + 0.2f * (rand() % 100) / 100.0f)
        };
        uint8_t cluster = kmeans_update(&model, f);
        assert(cluster == 1);  // Should assign to fault cluster
    }

    assert(model.k == 2);
    assert(model.total_points == 40);
}

TEST(memory_footprint) {
    printf("\n  Model size: %zu bytes", sizeof(kmeans_model_t));
    printf("\n  Cluster size: %zu bytes", sizeof(cluster_t));
    printf("\n  Max config (K=16, D=64): %zu bytes",
           sizeof(cluster_t) * MAX_CLUSTERS);

    // Verify under 100KB target
    assert(sizeof(kmeans_model_t) < 100 * 1024);
}

int main() {
    printf("=== Dynamic Clustering Tests ===\n");

    RUN_TEST(initialization_starts_k_equals_one);
    RUN_TEST(invalid_params);
    RUN_TEST(single_point_update_k_equals_one);
    RUN_TEST(add_cluster_increases_k);
    RUN_TEST(add_cluster_rejects_duplicate_label);
    RUN_TEST(add_cluster_enforces_max_k);
    RUN_TEST(prediction_matches_update);
    RUN_TEST(centroid_convergence_two_clusters);
    RUN_TEST(inertia_decreases_over_time);
    RUN_TEST(reset_returns_to_k_equals_one);
    RUN_TEST(high_dimensional_3d_accel);
    RUN_TEST(memory_footprint);

    printf("\n=== All Tests Passed ===\n");
    return 0;
}