/**
 * @file    sensors.c
 * @brief   Sensor Implementation — ADXL335 Vibration + LM35 Temperature
 *
 * This module handles all ADC hardware interaction.
 * Raw ADC values are scaled to physical units using datasheet-derived formulas.
 *
 * VIBRATION (ADXL335):
 *   Vout = (Vsupply/2) + (Sensitivity × Acceleration)
 *   Vsupply = 3.3V, Sensitivity ≈ 300mV/g
 *   g = (Vout_mV - 1650mV) / 300 mV/g
 *
 * TEMPERATURE (LM35):
 *   Vout = 10mV × Temperature(°C)
 *   Temp_C = Vout_mV / 10
 *
 * NOTE: ESP32 ADC has nonlinearity near 0V and 3.3V.
 * The 11dB attenuation + oneshot mode gives acceptable linearity
 * across the sensor's operating range.
 *
 * @author  Firmware Team — Agent 1
 */

#include "sensors.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>

/* =========================================================
 * PRIVATE VARIABLES
 * ========================================================= */
static const char *TAG = "SENSORS";

/* ADC handle — oneshot mode (preferred for periodic reads in FreeRTOS) */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/* ADC channel mappings (ADC1) */
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHAN_VIB    ADC_CHANNEL_0   /* GPIO36 */
#define ADC_CHAN_TEMP   ADC_CHANNEL_3   /* GPIO39 */

/* =========================================================
 * PRIVATE HELPER FUNCTIONS
 * ========================================================= */

/**
 * @brief Convert raw ADC count to millivolts
 *
 * Linear mapping: counts/4095 × Vref_mV
 * This ignores ESP32 nonlinearity for simplicity.
 * For production, use esp_adc_cal for calibrated readings.
 */
static inline float adc_raw_to_mv(int raw) {
    return ((float)raw / (float)ADC_MAX_VALUE) * (float)ADC_VREF_MV;
}

/**
 * @brief Convert ADC millivolts to g-force (ADXL335)
 */
static inline float mv_to_g(float mv) {
    return (mv - ADXL335_ZERO_G_OFFSET_MV) / ADXL335_SENSITIVITY_MV_PER_G;
}

/**
 * @brief Convert ADC millivolts to temperature (LM35)
 */
static inline float mv_to_celsius(float mv) {
    return mv / LM35_MV_PER_DEGREE;
}

/* =========================================================
 * PUBLIC API IMPLEMENTATION
 * ========================================================= */

esp_err_t sensors_init(void) {
    ESP_LOGI(TAG, "Initializing sensor ADC channels...");

    /* Configure ADC1 unit in oneshot mode */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    /* Configure vibration ADC channel */
    adc_oneshot_chan_cfg_t vib_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_11,   /* 0–3.9V input range */
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, ADC_CHAN_VIB, &vib_cfg));

    /* Configure temperature ADC channel */
    adc_oneshot_chan_cfg_t temp_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, ADC_CHAN_TEMP, &temp_cfg));

    ESP_LOGI(TAG, "Sensors initialized. Vib: GPIO%d, Temp: GPIO%d",
             PIN_VIBRATION_ADC, PIN_TEMPERATURE_ADC);
    return ESP_OK;
}

esp_err_t sensors_read(sensor_data_t *data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw_vib  = 0;
    int raw_temp = 0;

    /* Read raw ADC values */
    esp_err_t err_vib  = adc_oneshot_read(s_adc_handle, ADC_CHAN_VIB,  &raw_vib);
    esp_err_t err_temp = adc_oneshot_read(s_adc_handle, ADC_CHAN_TEMP, &raw_temp);

    if (err_vib != ESP_OK || err_temp != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed: vib=%d, temp=%d", err_vib, err_temp);
        data->valid = false;
        return ESP_FAIL;
    }

    /* Convert to engineering units */
    float vib_mv  = adc_raw_to_mv(raw_vib);
    float temp_mv = adc_raw_to_mv(raw_temp);

    data->vibration_g    = fabsf(mv_to_g(vib_mv));    /* Use absolute value — magnitude */
    data->temperature_c  = mv_to_celsius(temp_mv);
    data->timestamp_us   = esp_timer_get_time();
    data->valid          = true;

    /* Clamp vibration to 0 (noise floor protection) */
    if (data->vibration_g < 0.0f) {
        data->vibration_g = 0.0f;
    }

    return ESP_OK;
}

void sensors_deinit(void) {
    if (s_adc_handle != NULL) {
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
        ESP_LOGI(TAG, "ADC deinitialized");
    }
}
