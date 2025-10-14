/**
 * @file main.c
 * @brief RP2350 streaming k-means test
 * 
 * Generates synthetic 2D clusters and streams through model
 * View output: screen /dev/ttyACM0 115200
 */

#include "kmeans_platform.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <math.h>

// Simple PRNG (same as core implementation)
static uint32_t test_rng_state = 42;
static float rand_float() {
    test_rng_state = (1103515245 * test_rng_state + 12345) & 0x7FFFFFFF;
    return (float)test_rng_state / 0x7FFFFFFF;
}

// Generate 2D Gaussian point
static void generate_point(fixed_t* point, float cx, float cy, float std) {
    // Box-Muller transform
    float u1 = rand_float();
    float u2 = rand_float();
    float r = sqrtf(-2.0f * logf(u1 + 1e-10f));
    float theta = 2.0f * 3.14159f * u2;
    
    point[0] = FLOAT_TO_FIXED(cx + std * r * cosf(theta));
    point[1] = FLOAT_TO_FIXED(cy + std * r * sinf(theta));
}

int main() {
    kmeans_model_t model;
    
    // Initialize platform and model
    if (platform_init(&model, 3, 2, 0.2f) != PLATFORM_OK) {
        printf("FATAL: Platform initialization failed\n");
        while (1) {
            platform_led_blink(10, 50);  // Rapid blink = error
            sleep_ms(1000);
        }
    }
    
    printf("Starting streaming test...\n");
    printf("Generating 150 points from 3 clusters\n\n");
    
    // Stream points from 3 distinct clusters
    fixed_t point[2];
    
    for (uint16_t i = 0; i < 150; i++) {
        // Cluster A: (-1, -1)
        if (i % 3 == 0) {
            generate_point(point, -1.0f, -1.0f, 0.2f);
        }
        // Cluster B: (1, 1)
        else if (i % 3 == 1) {
            generate_point(point, 1.0f, 1.0f, 0.2f);
        }
        // Cluster C: (0, 0)
        else {
            generate_point(point, 0.0f, 0.0f, 0.2f);
        }
        
        platform_process_point(&model, point);
        
        // Print stats every 50 points
        if ((i + 1) % 50 == 0) {
            platform_print_stats(&model);
            sleep_ms(1000);  // Pause for readability
        }
    }
    
    printf("\n=== Test Complete ===\n");
    platform_print_stats(&model);
    
    // Success blink pattern (long-short-long)
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(2000);
    }
    
    return 0;
}