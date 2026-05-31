/**
 * @file    alert.h
 * @brief   Alert Management API — LED & Buzzer Control
 *
 * WHY THIS MODULE EXISTS:
 * In an industrial environment, operators cannot always watch a screen.
 * The LED and buzzer system provides immediate, non-intrusive on-site
 * feedback about machine health — independent of communication systems.
 *
 * ALERT BEHAVIOR:
 * - NORMAL:   Green LED ON,  buzzer OFF
 * - WARNING:  Yellow LED ON, buzzer slow pulse (1 Hz)
 * - CRITICAL: Red LED ON,    buzzer fast pulse (4 Hz) / continuous
 *
 * DESIGN: Buzzer uses ESP32 LEDC (PWM) to generate audible tones.
 * This avoids busy-waiting and is RTOS-safe.
 *
 * @author  Firmware Team — Agent 1
 */

#ifndef ALERT_H
#define ALERT_H

#include "config.h"

/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize GPIO pins for LEDs and LEDC for buzzer
 *
 * Configures LED GPIO as output push-pull.
 * Configures LEDC timer and channel for buzzer PWM.
 *
 * @return ESP_OK on success
 */
esp_err_t alert_init(void);

/**
 * @brief Update the alert outputs based on current machine health state
 *
 * This function is called by the alert task periodically.
 * It sets LED states and buzzer PWM duty/frequency accordingly.
 *
 * @param health  Current machine health state from anomaly detector
 */
void alert_update(system_health_t health);

/**
 * @brief Immediately silence all alerts (for manual override)
 *
 * Used during maintenance mode to suppress spurious alerts.
 */
void alert_silence(void);

/**
 * @brief Deinitialize alert hardware (for graceful shutdown)
 */
void alert_deinit(void);

#endif /* ALERT_H */
