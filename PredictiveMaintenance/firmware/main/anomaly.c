/**
 * @file    anomaly.c
 * @brief   Anomaly Detection Engine Implementation
 *
 * HYSTERESIS LOGIC EXPLANATION:
 *
 * Without hysteresis:
 *   If vibration = 1.01g → WARNING
 *   If vibration = 0.99g → NORMAL
 *   → Alert toggles hundreds of times per second near threshold!
 *
 * With hysteresis (ANOMALY_HYSTERESIS_VIB = 0.1g):
 *   Enter WARNING when: vibration > VIBRATION_WARNING_G (1.0g)
 *   Exit  WARNING when: vibration < VIBRATION_WARNING_G - hysteresis (0.9g)
 *   → Stable, clean state transitions.
 *
 * HEALTH PRIORITY: CRITICAL > WARNING > NORMAL
 * If either sensor is CRITICAL, machine state = CRITICAL.
 * If either sensor is WARNING (and none critical), state = WARNING.
 *
 * @author  Firmware Team — Agent 1
 */

#include "anomaly.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ANOMALY";

/* =========================================================
 * PRIVATE HELPER: Evaluate vibration severity (0.0–1.0)
 * Normalized: 0.0 = no vibration, 1.0 = at/above critical
 * ========================================================= */
static float compute_vib_severity(float vib_g) {
    if (vib_g >= VIBRATION_CRITICAL_G) return 1.0f;
    if (vib_g <= 0.0f)                 return 0.0f;
    return vib_g / VIBRATION_CRITICAL_G;
}

/* =========================================================
 * PRIVATE HELPER: Evaluate temperature severity (0.0–1.0)
 * ========================================================= */
static float compute_temp_severity(float temp_c) {
    if (temp_c >= TEMPERATURE_CRITICAL_C) return 1.0f;
    if (temp_c <= 0.0f)                   return 0.0f;
    return temp_c / TEMPERATURE_CRITICAL_C;
}

/* =========================================================
 * PUBLIC API
 * ========================================================= */

void anomaly_init(anomaly_state_t *state) {
    state->current_health  = HEALTH_NORMAL;
    state->vib_in_anomaly  = false;
    state->temp_in_anomaly = false;
    state->alert_count     = 0;
}

void anomaly_evaluate(anomaly_state_t       *state,
                      const anomaly_input_t *input,
                      anomaly_result_t      *result)
{
    float vib  = input->vibration_g;
    float temp = input->temperature_c;

    /* --- Vibration Hysteresis State Machine --- */
    if (!state->vib_in_anomaly) {
        /* Currently normal — check if threshold is exceeded */
        if (vib >= VIBRATION_WARNING_G) {
            state->vib_in_anomaly = true;
            state->alert_count++;
            ESP_LOGW(TAG, "Vibration anomaly detected: %.3f g", vib);
        }
    } else {
        /* Currently anomaly — check if value has recovered */
        if (vib < (VIBRATION_WARNING_G - ANOMALY_HYSTERESIS_VIB)) {
            state->vib_in_anomaly = false;
            ESP_LOGI(TAG, "Vibration returned to normal: %.3f g", vib);
        }
    }

    /* --- Temperature Hysteresis State Machine --- */
    if (!state->temp_in_anomaly) {
        if (temp >= TEMPERATURE_WARNING_C) {
            state->temp_in_anomaly = true;
            state->alert_count++;
            ESP_LOGW(TAG, "Temperature anomaly detected: %.1f C", temp);
        }
    } else {
        if (temp < (TEMPERATURE_WARNING_C - ANOMALY_HYSTERESIS_TEMP)) {
            state->temp_in_anomaly = false;
            ESP_LOGI(TAG, "Temperature returned to normal: %.1f C", temp);
        }
    }

    /* --- Determine Overall Machine Health --- */
    system_health_t new_health = HEALTH_NORMAL;

    if (vib >= VIBRATION_CRITICAL_G || temp >= TEMPERATURE_CRITICAL_C) {
        new_health = HEALTH_CRITICAL;
    } else if (state->vib_in_anomaly || state->temp_in_anomaly) {
        new_health = HEALTH_WARNING;
    }

    /* Log state transition */
    if (new_health != state->current_health) {
        ESP_LOGW(TAG, "Health state: %s → %s",
                 anomaly_health_to_string(state->current_health),
                 anomaly_health_to_string(new_health));
        state->current_health = new_health;
    }

    /* --- Fill Result Structure --- */
    result->health        = state->current_health;
    result->vib_anomaly   = state->vib_in_anomaly;
    result->temp_anomaly  = state->temp_in_anomaly;
    result->vib_severity  = compute_vib_severity(vib);
    result->temp_severity = compute_temp_severity(temp);
    result->last_alert_us = input->timestamp_us;
    result->alert_count   = state->alert_count;
}

const char *anomaly_health_to_string(system_health_t health) {
    switch (health) {
        case HEALTH_NORMAL:   return "NORMAL";
        case HEALTH_WARNING:  return "WARNING";
        case HEALTH_CRITICAL: return "CRITICAL";
        case HEALTH_FAULT:    return "FAULT";
        default:              return "UNKNOWN";
    }
}
