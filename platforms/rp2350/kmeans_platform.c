/**
 * @file kmeans_platform.c
 * @brief RP2350 platform implementation
 */

#include "kmeans_platform.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

platform_status_t platform_init(kmeans_model_t* model, uint8_t k,
                                uint8_t feature_dim, float learning_rate) {
    // Initialize USB serial for debug
    stdio_init_all();
    
    // Initialize WiFi chip (required for LED on Pico 2 W)
    if (cyw43_arch_init()) {
        printf("ERROR: WiFi chip initialization failed\n");
        return PLATFORM_ERROR_INIT;
    }
    
    // Blink LED to show we're alive
    platform_led_blink(3, 200);
    
    // Initialize k-means model
    if (!kmeans_init(model, k, feature_dim, learning_rate)) {
        printf("ERROR: Model initialization failed\n");
        return PLATFORM_ERROR_MODEL;
    }
    
    printf("\n=== RP2350 K-Means Platform ===\n");
    printf("Clusters: %d\n", k);
    printf("Features: %d\n", feature_dim);
    printf("Learning rate: %.3f\n", learning_rate);
    printf("Model size: %zu bytes\n", sizeof(kmeans_model_t));
    printf("================================\n\n");
    
    return PLATFORM_OK;
}

uint8_t platform_process_point(kmeans_model_t* model, const fixed_t* point) {
    // Update model
    uint8_t cluster_id = kmeans_update(model, point);
    
    // Blink LED once per point
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(10);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    
    // Log assignment
    printf("Point -> Cluster %d (total: %lu)\n", 
           cluster_id, model->total_points);
    
    return cluster_id;
}

void platform_print_stats(const kmeans_model_t* model) {
    if (!model->initialized) {
        printf("Model not initialized\n");
        return;
    }
    
    printf("\n--- Model Statistics ---\n");
    printf("Total points: %lu\n", model->total_points);
    printf("Inertia: %.3f\n", FIXED_TO_FLOAT(kmeans_inertia(model)));
    
    printf("\nCluster counts:\n");
    for (uint8_t i = 0; i < model->k; i++) {
        printf("  Cluster %d: %lu points (inertia: %.3f)\n",
               i, model->clusters[i].count,
               FIXED_TO_FLOAT(model->clusters[i].inertia));
    }
    
    printf("\nCentroids (first 4 features):\n");
    for (uint8_t i = 0; i < model->k; i++) {
        printf("  C%d: [", i);
        uint8_t display_dim = (model->feature_dim < 4) ? model->feature_dim : 4;
        for (uint8_t j = 0; j < display_dim; j++) {
            printf("%.3f", FIXED_TO_FLOAT(model->clusters[i].centroid[j]));
            if (j < display_dim - 1) printf(", ");
        }
        if (model->feature_dim > 4) {
            printf(", ... %d more", model->feature_dim - 4);
        }
        printf("]\n");
    }
    printf("------------------------\n\n");
}

void platform_led_blink(uint8_t times, uint32_t delay_ms) {
    for (uint8_t i = 0; i < times; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(delay_ms);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(delay_ms);
    }
}