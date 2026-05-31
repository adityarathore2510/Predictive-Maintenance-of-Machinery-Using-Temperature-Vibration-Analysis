/**
 * @file    anomaly.h
 * @brief   Anomaly Detection Engine API
 *
 * WHY THIS MODULE EXISTS:
 * This module implements threshold-based anomaly detection with hysteresis.
 * Hysteresis prevents alert "flickering" when sensor values hover around
 * a threshold boundary — a critical requirement in industrial control systems.
 *
 * DETECTION STRATEGY:
 * - Compare filtered sensor values against ISO-inspired thresholds
 * - Apply separate hysteresis on rising and falling edges
 * - Output machine health state: NORMAL / WARNING / CRITICAL / FAULT
 * - Track event timestamps for trend analysis
 *
 * @author  Firmware Team — Agent 1
 */

#ifndef ANOMALY_H
#define ANOMALY_H

#include "config.h"
#include <stdbool.h>
#include <stdint.h>

/* =========================================================
 * DATA STRUCTURES
 * ========================================================= */

/**
 * @brief Input to the anomaly detector — filtered sensor values
 */
typedef struct {
    float   vibration_g;     /* Filtered vibration magnitude (g) */
    float   temperature_c;   /* Filtered temperature (°C)        */
    int64_t timestamp_us;    /* Sample timestamp                 */
} anomaly_input_t;

/**
 * @brief Output of the anomaly detector — health assessment
 */
typedef struct {
    system_health_t health;          /* Overall machine health state   */
    bool            vib_anomaly;     /* True if vibration is abnormal  */
    bool            temp_anomaly;    /* True if temperature is abnormal */
    float           vib_severity;    /* 0.0–1.0 normalized severity    */
    float           temp_severity;   /* 0.0–1.0 normalized severity    */
    int64_t         last_alert_us;   /* Timestamp of last alert event  */
    uint32_t        alert_count;     /* Total alerts since boot        */
} anomaly_result_t;

/**
 * @brief Internal state for the detector (for hysteresis tracking)
 */
typedef struct {
    system_health_t current_health;     /* Current latched health state */
    bool            vib_in_anomaly;     /* Latched vibration state      */
    bool            temp_in_anomaly;    /* Latched temperature state    */
    uint32_t        alert_count;        /* Persistent alert counter     */
} anomaly_state_t;

/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize anomaly detection state
 *
 * @param[out] state  Pointer to anomaly_state_t to initialize
 */
void anomaly_init(anomaly_state_t *state);

/**
 * @brief Evaluate sensor readings and produce health assessment
 *
 * Applies hysteresis logic:
 * - To enter WARNING: value must exceed warning threshold
 * - To return to NORMAL: value must drop below (warning threshold - hysteresis)
 *
 * @param[in,out] state   Internal state (tracks hysteresis)
 * @param[in]     input   Filtered sensor readings
 * @param[out]    result  Health assessment output
 */
void anomaly_evaluate(anomaly_state_t    *state,
                      const anomaly_input_t *input,
                      anomaly_result_t   *result);

/**
 * @brief Convert health state enum to human-readable string
 *
 * @param health  Health state
 * @return  String label (e.g., "NORMAL", "WARNING", "CRITICAL")
 */
const char *anomaly_health_to_string(system_health_t health);

#endif /* ANOMALY_H */
