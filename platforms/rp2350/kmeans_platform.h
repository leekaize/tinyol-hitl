/**
 * @file kmeans_platform.h
 * @brief RP2350 platform wrapper for streaming k-means
 * 
 * Hardware: Raspberry Pi Pico 2 (RP2350)
 * Clock: 150MHz default
 * RAM: 520KB available
 */

#ifndef KMEANS_PLATFORM_H
#define KMEANS_PLATFORM_H

#include "../../core/clustering/streaming_kmeans.h"
#include "pico/stdlib.h"

// Platform configuration
#define UART_BAUD 115200        // Debug output

// Status codes
typedef enum {
    PLATFORM_OK = 0,
    PLATFORM_ERROR_INIT = -1,
    PLATFORM_ERROR_MODEL = -2
} platform_status_t;

/**
 * Initialize RP2350 hardware and k-means model
 * @param model Model to initialize
 * @param k Number of clusters
 * @param feature_dim Feature dimension
 * @param learning_rate Learning rate
 * @return Platform status
 */
platform_status_t platform_init(kmeans_model_t* model, uint8_t k, 
                                uint8_t feature_dim, float learning_rate);

/**
 * Process single point and log result to UART
 * @param model Model to update
 * @param point Feature vector
 * @return Assigned cluster ID
 */
uint8_t platform_process_point(kmeans_model_t* model, const fixed_t* point);

/**
 * Print model statistics to UART
 * @param model Model to inspect
 */
void platform_print_stats(const kmeans_model_t* model);

/**
 * Blink LED to indicate activity
 * @param times Number of blinks
 * @param delay_ms Delay between blinks
 */
void platform_led_blink(uint8_t times, uint32_t delay_ms);

#endif // KMEANS_PLATFORM_H