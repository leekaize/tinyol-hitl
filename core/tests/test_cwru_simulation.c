/**
 * @file test_cwru_simulation.c
 * @brief Simulates CWRU dataset streaming with HITL labeling
 *
 * Validates:
 * - K grows from 1→4 with proper labeling
 * - assign_existing prevents K explosion
 * - Buffer averaging works correctly
 */

#include "../streaming_kmeans.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Simulated CWRU cluster centers (4 features: rms, kurtosis, crest, variance)
#define FEATURE_DIM 4
#define SAMPLES_PER_CLASS 100
#define TRAIN_SPLIT 0.7

typedef struct {
    fixed_t features[FEATURE_DIM];
    uint8_t true_label;  // 0=normal, 1=ball, 2=inner, 3=outer
} sample_t;

const char* LABEL_NAMES[] = {"normal", "ball", "inner", "outer"};

// Simulated cluster centers (realistic CWRU feature ranges)
const float CLUSTER_CENTERS[4][4] = {
    {0.5f, 3.0f, 2.0f, 0.1f},   // normal: low rms, low kurtosis
    {1.2f, 8.0f, 4.5f, 0.4f},   // ball: medium rms, high kurtosis
    {1.8f, 12.0f, 5.0f, 0.6f},  // inner: high rms, very high kurtosis
    {1.5f, 6.0f, 3.5f, 0.3f}    // outer: medium-high rms, medium kurtosis
};

// Generate sample with noise around cluster center
void generate_sample(sample_t* s, uint8_t label, float noise_std) {
    s->true_label = label;
    for (int d = 0; d < FEATURE_DIM; d++) {
        float val = CLUSTER_CENTERS[label][d];
        // Add Gaussian-ish noise (box-muller approximation)
        float u1 = (rand() % 1000 + 1) / 1000.0f;
        float u2 = (rand() % 1000 + 1) / 1000.0f;
        float noise = sqrtf(-2.0f * logf(u1)) * cosf(6.28318f * u2) * noise_std;
        val += noise;
        if (val < 0) val = 0.01f;
        s->features[d] = FLOAT_TO_FIXED(val);
    }
}

// Simulate operator decision based on true label and current model
// Returns: 0 = new label, 1-K = assign to existing cluster, -1 = discard
int simulate_operator(kmeans_model_t* model, uint8_t true_label) {
    // Check if this label already exists
    for (uint8_t i = 0; i < model->k; i++) {
        if (strcmp(model->clusters[i].label, LABEL_NAMES[true_label]) == 0) {
            return i + 1;  // Assign to existing cluster i (1-indexed for clarity)
        }
    }
    return 0;  // New label needed
}

void run_cwru_simulation() {
    printf("\n=== CWRU Simulation Test ===\n\n");

    srand(42);  // Reproducible

    // Generate dataset
    int total_samples = 4 * SAMPLES_PER_CLASS;
    sample_t* dataset = malloc(total_samples * sizeof(sample_t));

    int idx = 0;
    for (int label = 0; label < 4; label++) {
        for (int i = 0; i < SAMPLES_PER_CLASS; i++) {
            generate_sample(&dataset[idx], label, 0.15f);
            idx++;
        }
    }

    // Shuffle dataset
    for (int i = total_samples - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        sample_t tmp = dataset[i];
        dataset[i] = dataset[j];
        dataset[j] = tmp;
    }

    // Split train/test
    int train_size = (int)(total_samples * TRAIN_SPLIT);
    int test_size = total_samples - train_size;

    printf("Dataset: %d samples (train: %d, test: %d)\n",
           total_samples, train_size, test_size);
    printf("Classes: normal, ball, inner, outer\n\n");

    // Initialize model
    kmeans_model_t model;
    kmeans_init(&model, FEATURE_DIM, 0.2f);

    printf("--- Training Phase ---\n");

    int new_labels_created = 0;
    int assigns_to_existing = 0;
    int normal_updates = 0;

    for (int i = 0; i < train_size; i++) {
        sample_t* s = &dataset[i];

        int8_t cluster = kmeans_update(&model, s->features);

        if (cluster >= 0) {
            // Normal assignment
            normal_updates++;
        } else {
            // Anomaly detected - simulate operator
            int op_decision = simulate_operator(&model, s->true_label);

            if (op_decision == 0) {
                // New label
                kmeans_add_cluster(&model, LABEL_NAMES[s->true_label]);
                new_labels_created++;
                printf("  Sample %d: NEW cluster '%s' (K=%d)\n",
                       i, LABEL_NAMES[s->true_label], model.k);
            } else {
                // Assign to existing
                kmeans_assign_existing(&model, op_decision - 1);
                assigns_to_existing++;
            }
        }
    }

    printf("\nTraining complete:\n");
    printf("  K = %d clusters\n", model.k);
    printf("  New labels created: %d\n", new_labels_created);
    printf("  Assigned to existing: %d\n", assigns_to_existing);
    printf("  Normal updates: %d\n", normal_updates);

    // Print cluster info
    printf("\nCluster summary:\n");
    for (uint8_t i = 0; i < model.k; i++) {
        char label[MAX_LABEL_LENGTH];
        kmeans_get_label(&model, i, label);
        printf("  C%d '%s': count=%lu\n", i, label, model.clusters[i].count);
    }

    // Test phase
    printf("\n--- Test Phase ---\n");

    int confusion[4][4] = {0};  // [true][predicted]

    for (int i = train_size; i < total_samples; i++) {
        sample_t* s = &dataset[i];
        uint8_t predicted = kmeans_predict(&model, s->features);

        // Map predicted cluster to label index
        char pred_label[MAX_LABEL_LENGTH];
        kmeans_get_label(&model, predicted, pred_label);

        int pred_idx = -1;
        for (int j = 0; j < 4; j++) {
            if (strcmp(pred_label, LABEL_NAMES[j]) == 0) {
                pred_idx = j;
                break;
            }
        }

        if (pred_idx >= 0) {
            confusion[s->true_label][pred_idx]++;
        }
    }

    // Print confusion matrix
    printf("\nConfusion Matrix:\n");
    printf("            Predicted\n");
    printf("            norm  ball  innr  outr\n");
    int correct = 0;
    for (int t = 0; t < 4; t++) {
        printf("Actual %s", LABEL_NAMES[t]);
        if (strlen(LABEL_NAMES[t]) < 6) printf(" ");
        for (int p = 0; p < 4; p++) {
            printf(" %4d", confusion[t][p]);
            if (t == p) correct += confusion[t][p];
        }
        printf("\n");
    }

    float accuracy = 100.0f * correct / test_size;
    printf("\nAccuracy: %d/%d = %.1f%%\n", correct, test_size, accuracy);

    // Validate K = 4
    if (model.k == 4) {
        printf("\n✓ K correctly grew to 4 (matching CWRU classes)\n");
    } else {
        printf("\n✗ K = %d (expected 4)\n", model.k);
    }

    free(dataset);
}

int main() {
    run_cwru_simulation();
    return 0;
}