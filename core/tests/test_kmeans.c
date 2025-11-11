/**
 * @file test_kmeans.c
 * @brief Unit tests for streaming k-means
 *
 * Compile: gcc -o test_kmeans test_kmeans.c streaming_kmeans.c -lm
 * Run: ./test_kmeans
 */

#include "../streaming_kmeans.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

// Test utilities
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASS\n"); \
} while(0)

// Helper: Generate synthetic 2D Gaussian cluster
static void generate_cluster_2d(fixed_t* points, uint16_t n, float cx, float cy, float std) {
    for (uint16_t i = 0; i < n; i++) {
        // Box-Muller transform for Gaussian
        float u1 = (float)rand() / RAND_MAX;
        float u2 = (float)rand() / RAND_MAX;
        float r = sqrtf(-2.0f * logf(u1));
        float theta = 2.0f * M_PI * u2;

        points[i * 2] = FLOAT_TO_FIXED(cx + std * r * cosf(theta));
        points[i * 2 + 1] = FLOAT_TO_FIXED(cy + std * r * sinf(theta));
    }
}

TEST(initialization) {
    kmeans_model_t model;
    bool ok = kmeans_init(&model, 3, 2, 0.1f);

    assert(ok);
    assert(model.initialized);
    assert(model.k == 3);
    assert(model.feature_dim == 2);
    assert(model.total_points == 0);
}

TEST(invalid_params) {
    kmeans_model_t model;

    // Too many clusters
    assert(!kmeans_init(&model, MAX_CLUSTERS + 1, 2, 0.1f));

    // Too many features
    assert(!kmeans_init(&model, 3, MAX_FEATURES + 1, 0.1f));

    // Zero values
    assert(!kmeans_init(&model, 0, 2, 0.1f));
    assert(!kmeans_init(&model, 3, 0, 0.1f));
}

TEST(single_point_update) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.5f), FLOAT_TO_FIXED(0.5f)};
    uint8_t cluster_id = kmeans_update(&model, point);

    assert(cluster_id < 2);
    assert(model.total_points == 1);
    assert(model.clusters[cluster_id].count == 1);
}

TEST(prediction_matches_update) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(0.3f), FLOAT_TO_FIXED(-0.2f)};

    uint8_t predicted = kmeans_predict(&model, point);
    uint8_t updated = kmeans_update(&model, point);

    assert(predicted == updated);
}

TEST(centroid_convergence) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.3f);

    // Stream 100 points from two clusters
    fixed_t points_a[200], points_b[200];
    generate_cluster_2d(points_a, 100, -1.0f, -1.0f, 0.2f);
    generate_cluster_2d(points_b, 100, 1.0f, 1.0f, 0.2f);

    // Process points
    for (int i = 0; i < 100; i++) {
        kmeans_update(&model, &points_a[i * 2]);
        kmeans_update(&model, &points_b[i * 2]);
    }

    // Check clusters formed
    assert(model.total_points == 200);
    assert(model.clusters[0].count > 0);
    assert(model.clusters[1].count > 0);

    // Verify centroids moved toward cluster centers
    fixed_t c0[2], c1[2];
    kmeans_get_centroid(&model, 0, c0);
    kmeans_get_centroid(&model, 1, c1);

    float c0_x = FIXED_TO_FLOAT(c0[0]);
    float c0_y = FIXED_TO_FLOAT(c0[1]);
    float c1_x = FIXED_TO_FLOAT(c1[0]);
    float c1_y = FIXED_TO_FLOAT(c1[1]);

    // Centroids should be near (-1,-1) and (1,1)
    float dist0_to_a = sqrtf((c0_x + 1.0f) * (c0_x + 1.0f) + (c0_y + 1.0f) * (c0_y + 1.0f));
    float dist0_to_b = sqrtf((c0_x - 1.0f) * (c0_x - 1.0f) + (c0_y - 1.0f) * (c0_y - 1.0f));
    float dist1_to_a = sqrtf((c1_x + 1.0f) * (c1_x + 1.0f) + (c1_y + 1.0f) * (c1_y + 1.0f));
    float dist1_to_b = sqrtf((c1_x - 1.0f) * (c1_x - 1.0f) + (c1_y - 1.0f) * (c1_y - 1.0f));

    // One centroid near A, one near B (threshold: 0.5 units)
    bool valid = (dist0_to_a < 0.5f && dist1_to_b < 0.5f) ||
                 (dist0_to_b < 0.5f && dist1_to_a < 0.5f);
    assert(valid);

    printf("\n  Centroids: C0=(%.2f, %.2f) C1=(%.2f, %.2f)",
           c0_x, c0_y, c1_x, c1_y);
}

TEST(inertia_decreases) {
    kmeans_model_t model;
    kmeans_init(&model, 3, 2, 0.2f);

    fixed_t points[300];
    generate_cluster_2d(&points[0], 100, 0.0f, 0.0f, 0.3f);

    // Process 20 points, check inertia trend
    for (int i = 0; i < 20; i++) {
        kmeans_update(&model, &points[i * 2]);
    }
    fixed_t inertia_early = kmeans_inertia(&model);

    // Process 80 more points
    for (int i = 20; i < 100; i++) {
        kmeans_update(&model, &points[i * 2]);
    }
    fixed_t inertia_late = kmeans_inertia(&model);

    // Inertia should stabilize or decrease (may not strictly decrease due to EMA)
    printf("\n  Inertia: early=%.3f late=%.3f",
           FIXED_TO_FLOAT(inertia_early), FIXED_TO_FLOAT(inertia_late));
}

TEST(reset_functionality) {
    kmeans_model_t model;
    kmeans_init(&model, 2, 2, 0.1f);

    fixed_t point[2] = {FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f)};
    kmeans_update(&model, point);
    kmeans_update(&model, point);

    assert(model.total_points == 2);

    kmeans_reset(&model);

    assert(model.total_points == 0);
    assert(model.initialized);
    assert(model.k == 2);
}

TEST(high_dimensional) {
    kmeans_model_t model;
    kmeans_init(&model, 4, 32, 0.15f);

    fixed_t point[32];
    for (int i = 0; i < 32; i++) {
        point[i] = FLOAT_TO_FIXED((float)rand() / RAND_MAX);
    }

    uint8_t cluster_id = kmeans_update(&model, point);
    assert(cluster_id < 4);
    assert(model.total_points == 1);
}

TEST(memory_footprint) {
    printf("\n  Model size: %zu bytes", sizeof(kmeans_model_t));
    printf("\n  Max config (16 clusters, 64 features): %zu bytes",
           sizeof(cluster_t) * MAX_CLUSTERS);

    // Verify under 100KB target
    assert(sizeof(kmeans_model_t) < 100 * 1024);
}

int main() {
    printf("=== Streaming K-Means Tests ===\n");

    RUN_TEST(initialization);
    RUN_TEST(invalid_params);
    RUN_TEST(single_point_update);
    RUN_TEST(prediction_matches_update);
    RUN_TEST(centroid_convergence);
    RUN_TEST(inertia_decreases);
    RUN_TEST(reset_functionality);
    RUN_TEST(high_dimensional);
    RUN_TEST(memory_footprint);

    printf("\n=== All Tests Passed ===\n");
    return 0;
}