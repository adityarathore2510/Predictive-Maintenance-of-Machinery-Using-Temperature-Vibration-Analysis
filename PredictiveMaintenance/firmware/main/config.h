/**
 * @file    config.h
 * @brief   Global System Configuration — Predictive Maintenance System
 *
 * Central configuration file for all hardware pins, threshold values,
 * filter parameters, and FreeRTOS task priorities.
 *
 * DESIGN DECISION: All tunable parameters are defined here so that
 * any hardware or threshold change requires a single-file edit,
 * keeping the rest of the codebase clean and portable.
 *
 * @author  Firmware Team — Agent 1
 * @version 1.0
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "freertos/FreeRTOS.h"

/* =========================================================
 * SYSTEM IDENTITY
 * ========================================================= */
#define FIRMWARE_VERSION        "1.0.0"
#define DEVICE_ID               "ESP32-PM-01"    /* Machine/Node ID for MQTT topics */

/* =========================================================
 * HARDWARE PIN CONFIGURATION
 * ESP32 DevKit V1 (38-pin)
 * ========================================================= */

/* Analog Input Pins (ADC1 channel — GPIO 32-39 are ADC1, safe with WiFi) */
#define PIN_VIBRATION_ADC       36   /* GPIO36 / ADC1_CH0 — ADXL335 Z-axis analog output */
#define PIN_TEMPERATURE_ADC     39   /* GPIO39 / ADC1_CH3 — LM35 analog output           */

/* Digital Output Pins — Alert System */
#define PIN_BUZZER              25   /* GPIO25 — PWM buzzer (LEDC channel)   */
#define PIN_LED_NORMAL          26   /* GPIO26 — Green LED — normal operation */
#define PIN_LED_WARNING         27   /* GPIO27 — Yellow LED — warning state  */
#define PIN_LED_CRITICAL        14   /* GPIO14 — Red LED — critical/fault state */

/* =========================================================
 * ADC CONFIGURATION
 * ESP32 ADC is 12-bit (0–4095), Vref ≈ 3.3V
 * ========================================================= */
#define ADC_RESOLUTION_BITS     12
#define ADC_MAX_VALUE           4095
#define ADC_VREF_MV             3300   /* millivolts */

/* LM35 sensitivity: 10mV/°C, output at 0°C = 0V */
#define LM35_MV_PER_DEGREE      10.0f

/* ADXL335: sensitivity ~300mV/g, zero-g offset at Vcc/2 = 1650mV */
#define ADXL335_SENSITIVITY_MV_PER_G   300.0f
#define ADXL335_ZERO_G_OFFSET_MV      1650.0f

/* =========================================================
 * MOVING AVERAGE FILTER CONFIGURATION
 * Larger window = smoother but slower response
 * DESIGN DECISION: 10-sample window at 50ms interval = 500ms averaging
 * This filters mechanical vibration noise without masking real faults
 * ========================================================= */
#define FILTER_WINDOW_SIZE      10

/* =========================================================
 * ANOMALY DETECTION THRESHOLDS
 * Based on ISO 10816 vibration severity guidelines (simplified)
 * and typical industrial machine temperature operating ranges
 * ========================================================= */

/* Vibration thresholds in g-force */
#define VIBRATION_NORMAL_G      0.5f    /* Below this: healthy machine */
#define VIBRATION_WARNING_G     1.0f    /* Warning zone: increased monitoring */
#define VIBRATION_CRITICAL_G    2.0f    /* Critical: imminent bearing failure */

/* Temperature thresholds in degrees Celsius */
#define TEMPERATURE_NORMAL_C    40.0f   /* Normal operating temperature */
#define TEMPERATURE_WARNING_C   65.0f   /* Warning: thermal stress increasing */
#define TEMPERATURE_CRITICAL_C  80.0f   /* Critical: thermal runaway risk */

/* Hysteresis margin to prevent alert flickering */
#define ANOMALY_HYSTERESIS_VIB  0.1f    /* ±0.1g hysteresis band */
#define ANOMALY_HYSTERESIS_TEMP 2.0f    /* ±2°C hysteresis band   */

/* =========================================================
 * FREERTOS TASK CONFIGURATION
 * DESIGN DECISION: Priority ladder ensures sensor acquisition
 * has highest priority to maintain deterministic sampling rate.
 * ========================================================= */
#define TASK_PRIORITY_SENSOR    5   /* Highest — deterministic sampling */
#define TASK_PRIORITY_FILTER    4   /* Signal processing */
#define TASK_PRIORITY_ANOMALY   3   /* Anomaly detection */
#define TASK_PRIORITY_ALERT     3   /* Alert management (same as anomaly) */
#define TASK_PRIORITY_COMM      2   /* Serial/MQTT output — lowest priority */

/* Task stack sizes in words (1 word = 4 bytes on Xtensa LX6) */
#define TASK_STACK_SENSOR       2048
#define TASK_STACK_FILTER       2048
#define TASK_STACK_ANOMALY      2048
#define TASK_STACK_ALERT        2048
#define TASK_STACK_COMM         4096  /* Larger for JSON string formatting */

/* Sampling interval — sensor task delay */
#define SENSOR_SAMPLE_INTERVAL_MS   50    /* 20 Hz sampling rate */
#define COMM_TRANSMIT_INTERVAL_MS   500   /* Transmit every 500ms (averaged data) */

/* =========================================================
 * COMMUNICATION CONFIGURATION
 * ========================================================= */
#define SERIAL_BAUD_RATE        115200

/* MQTT Configuration (optional — compile with -DENABLE_MQTT) */
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
#define MQTT_TOPIC_SENSOR       "plant/machine01/sensors"
#define MQTT_TOPIC_ALERT        "plant/machine01/alerts"
#define MQTT_QOS                1

/* =========================================================
 * SYSTEM HEALTH STATES
 * ========================================================= */
typedef enum {
    HEALTH_NORMAL   = 0,
    HEALTH_WARNING  = 1,
    HEALTH_CRITICAL = 2,
    HEALTH_FAULT    = 3
} system_health_t;

#endif /* CONFIG_H */
