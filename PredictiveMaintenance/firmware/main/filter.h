/**
 * @file    filter.h
 * @brief   Moving Average Filter API
 *
 * WHY THIS MODULE EXISTS:
 * Raw ADC readings from vibration sensors contain high-frequency noise
 * due to electrical interference and mechanical vibration harmonics.
 * A moving average filter smooths readings while preserving the
 * trend information needed for anomaly detection.
 *
 * DESIGN: Uses a fixed-size circular buffer — O(1) update, O(1) read.
 * No dynamic memory allocation — safe for embedded RTOS environments.
 *
 * @author  Firmware Team — Agent 1
 */

#ifndef FILTER_H
#define FILTER_H

#include "config.h"
#include <stdint.h>

/* =========================================================
 * DATA STRUCTURES
 * ========================================================= */

/**
 * @brief Circular buffer filter state
 *
 * One instance is required per sensor channel.
 * All fields are private — use API functions only.
 */
typedef struct {
    float   buffer[FILTER_WINDOW_SIZE]; /* Circular sample buffer      */
    uint8_t head;                       /* Current write position      */
    uint8_t count;                      /* Number of valid samples     */
    float   sum;                        /* Running sum for O(1) average */
} moving_avg_filter_t;

/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize a filter instance (zero all state)
 *
 * Must be called before the first filter_update() call.
 *
 * @param[out] filter  Pointer to filter instance
 */
void filter_init(moving_avg_filter_t *filter);

/**
 * @brief Add a new sample and return the updated average
 *
 * Uses a running sum for efficient O(1) computation.
 * When the buffer is not yet full, averages only valid samples.
 *
 * @param[in,out] filter  Pointer to filter instance
 * @param[in]     sample  New sensor reading
 * @return  Current moving average
 */
float filter_update(moving_avg_filter_t *filter, float sample);

/**
 * @brief Get the current average without adding a new sample
 *
 * @param[in] filter  Pointer to filter instance
 * @return  Current moving average (0.0f if no samples)
 */
float filter_get_average(const moving_avg_filter_t *filter);

/**
 * @brief Reset the filter (clear all accumulated samples)
 *
 * @param[out] filter  Pointer to filter instance
 */
void filter_reset(moving_avg_filter_t *filter);

#endif /* FILTER_H */
