/**
 * @file    filter.c
 * @brief   Moving Average Filter Implementation
 *
 * Circular buffer implementation for real-time signal smoothing.
 *
 * ALGORITHM:
 *   - Maintains a circular buffer of last N samples
 *   - Keeps a running sum: when a new sample arrives,
 *     subtract the oldest sample (being overwritten) and add the new one
 *   - Average = sum / count
 *
 * This approach gives O(1) time complexity per update vs O(N) naive sum.
 *
 * @author  Firmware Team — Agent 1
 */

#include "filter.h"
#include <string.h>

/* =========================================================
 * PUBLIC API IMPLEMENTATION
 * ========================================================= */

void filter_init(moving_avg_filter_t *filter) {
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->head  = 0;
    filter->count = 0;
    filter->sum   = 0.0f;
}

float filter_update(moving_avg_filter_t *filter, float sample) {
    /* If buffer is full, subtract the oldest value before overwriting */
    if (filter->count == FILTER_WINDOW_SIZE) {
        filter->sum -= filter->buffer[filter->head];
    } else {
        /* Buffer not yet full — increment valid count */
        filter->count++;
    }

    /* Write new sample at head position */
    filter->buffer[filter->head] = sample;
    filter->sum += sample;

    /* Advance head (circular wraparound) */
    filter->head = (filter->head + 1) % FILTER_WINDOW_SIZE;

    /* Return current average */
    return (filter->count > 0) ? (filter->sum / (float)filter->count) : 0.0f;
}

float filter_get_average(const moving_avg_filter_t *filter) {
    return (filter->count > 0) ? (filter->sum / (float)filter->count) : 0.0f;
}

void filter_reset(moving_avg_filter_t *filter) {
    filter_init(filter);
}
