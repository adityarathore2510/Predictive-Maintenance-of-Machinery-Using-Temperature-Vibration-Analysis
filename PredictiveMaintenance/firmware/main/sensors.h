/**
 * @file    sensors.h
 * @brief   Sensor Interface API — Vibration & Temperature
 *
 * Provides a clean abstraction layer over ESP32 ADC hardware.
 * Physical sensors are converted to engineering units here.
 *
 * WHY THIS MODULE EXISTS:
 * Separating hardware-specific ADC code from application logic
 * makes the firmware portable and testable. If the sensor type
 * changes (e.g., LM35 → DS18B20), only this file changes.
 *
 * @author  Firmware Team — Agent 1
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include <stdbool.h>

/* =========================================================
 * DATA STRUCTURES
 * ========================================================= */

/**
 * @brief Raw sensor readings directly from ADC
 */
typedef struct {
    int     vibration_raw;      /* ADC counts (0–4095) */
    int     temperature_raw;    /* ADC counts (0–4095) */
    int64_t timestamp_us;       /* esp_timer_get_time() — microseconds */
} sensor_raw_t;

/**
 * @brief Sensor readings converted to engineering units
 */
typedef struct {
    float   vibration_g;        /* Vibration in g-force */
    float   temperature_c;      /* Temperature in °C    */
    int64_t timestamp_us;       /* Timestamp in microseconds */
    bool    valid;              /* True if reading is valid */
} sensor_data_t;

/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize ADC hardware for both sensor channels
 *
 * Configures ADC1 in oneshot mode with 12-bit resolution and
 * 11dB attenuation (allows reading 0–3.9V range safely).
 *
 * @return ESP_OK on success, ESP_FAIL on hardware error
 */
esp_err_t sensors_init(void);

/**
 * @brief Read all sensors and convert to engineering units
 *
 * Performs a single ADC read for vibration and temperature,
 * converts raw ADC counts to g-force and Celsius respectively.
 *
 * @param[out] data  Pointer to sensor_data_t output structure
 * @return ESP_OK on success
 */
esp_err_t sensors_read(sensor_data_t *data);

/**
 * @brief Deinitialize ADC resources (for graceful shutdown)
 */
void sensors_deinit(void);

#endif /* SENSORS_H */
