/**
 * @file    main.c
 * @brief   FreeRTOS Task Orchestration — Predictive Maintenance System
 *
 * SYSTEM ARCHITECTURE OVERVIEW:
 * This file is the entry point and task orchestrator. It creates all
 * FreeRTOS tasks and inter-task communication primitives (queues/semaphores).
 *
 * TASK FLOW:
 *
 *   ┌─────────────────────────────────────────────────────┐
 *   │  task_sensor  (50ms)                                │
 *   │  → Reads ADC, converts to engineering units         │
 *   │  → Pushes sensor_data_t to xQueueSensorData         │
 *   └────────────────────┬────────────────────────────────┘
 *                        │ xQueueSensorData
 *   ┌────────────────────▼────────────────────────────────┐
 *   │  task_filter  (consumer of SensorData)              │
 *   │  → Applies moving average to vibration & temp       │
 *   │  → Pushes filtered anomaly_input_t to xQueueFiltered│
 *   └────────────────────┬────────────────────────────────┘
 *                        │ xQueueFiltered
 *   ┌────────────────────▼────────────────────────────────┐
 *   │  task_anomaly (consumer of Filtered)                │
 *   │  → Evaluates thresholds with hysteresis             │
 *   │  → Updates shared g_current_result (protected by    │
 *   │    xSemaphoreResult)                                │
 *   └─────────────────────────────────────────────────────┘
 *        │                                   │
 *   ┌────▼────────────┐         ┌────────────▼────────────┐
 *   │  task_alert     │         │  task_comm (500ms)      │
 *   │  (100ms)        │         │  → Reads g_current_result│
 *   │  → Updates LED  │         │  → Formats JSON          │
 *   │  → Controls     │         │  → Transmits via UART2   │
 *   │    buzzer PWM   │         │                          │
 *   └─────────────────┘         └──────────────────────────┘
 *
 * WHY FREERTOS QUEUES:
 * Queues decouple producers from consumers and eliminate race conditions.
 * The sensor task does not need to know anything about filtering logic.
 *
 * @author  Firmware Team — Agent 1
 * @version 1.0.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "config.h"
#include "sensors.h"
#include "filter.h"
#include "anomaly.h"
#include "alert.h"
#include "comm.h"

/* =========================================================
 * LOGGING TAG
 * ========================================================= */
static const char *TAG = "MAIN";

/* =========================================================
 * INTER-TASK COMMUNICATION
 * Queue depth = 4: allows brief bursts without data loss
 * while keeping memory footprint low.
 * ========================================================= */
static QueueHandle_t     xQueueSensorData  = NULL;  /* sensor_data_t packets */
static QueueHandle_t     xQueueFiltered    = NULL;  /* anomaly_input_t packets */
static SemaphoreHandle_t xSemaphoreResult  = NULL;  /* Protects g_current_result */

/* =========================================================
 * SHARED RESULT (written by anomaly task, read by alert+comm)
 * ========================================================= */
static anomaly_result_t  g_current_result;
static sensor_data_t     g_last_filtered;   /* For comm task reference */

/* =========================================================
 * TASK: SENSOR ACQUISITION
 * Priority: TASK_PRIORITY_SENSOR (highest)
 * Period:   SENSOR_SAMPLE_INTERVAL_MS (50ms = 20Hz)
 * ========================================================= */
static void task_sensor(void *pvParameters) {
    ESP_LOGI(TAG, "Sensor task started on core %d", xPortGetCoreID());
    sensor_data_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        if (sensors_read(&data) == ESP_OK && data.valid) {
            /* Enqueue — overwrite oldest if queue full (non-blocking) */
            if (xQueueSend(xQueueSensorData, &data, 0) != pdTRUE) {
                ESP_LOGD(TAG, "Sensor queue full — sample dropped");
            }
        }
        /* Precise periodic delay — vTaskDelayUntil prevents drift */
        vTaskDelayUntil(&xLastWakeTime,
                        pdMS_TO_TICKS(SENSOR_SAMPLE_INTERVAL_MS));
    }
}

/* =========================================================
 * TASK: SIGNAL FILTERING
 * Priority: TASK_PRIORITY_FILTER
 * ========================================================= */
static void task_filter(void *pvParameters) {
    ESP_LOGI(TAG, "Filter task started");

    /* Each sensor channel gets its own filter instance */
    moving_avg_filter_t vib_filter;
    moving_avg_filter_t temp_filter;
    filter_init(&vib_filter);
    filter_init(&temp_filter);

    sensor_data_t  raw;
    anomaly_input_t filtered;

    while (1) {
        /* Block indefinitely until a sensor sample arrives */
        if (xQueueReceive(xQueueSensorData, &raw, portMAX_DELAY) == pdTRUE) {
            filtered.vibration_g   = filter_update(&vib_filter,  raw.vibration_g);
            filtered.temperature_c = filter_update(&temp_filter, raw.temperature_c);
            filtered.timestamp_us  = raw.timestamp_us;

            xQueueSend(xQueueFiltered, &filtered, 0);
        }
    }
}

/* =========================================================
 * TASK: ANOMALY DETECTION
 * Priority: TASK_PRIORITY_ANOMALY
 * ========================================================= */
static void task_anomaly(void *pvParameters) {
    ESP_LOGI(TAG, "Anomaly detection task started");

    anomaly_state_t  state;
    anomaly_result_t result;
    anomaly_input_t  filtered;
    anomaly_init(&state);

    while (1) {
        if (xQueueReceive(xQueueFiltered, &filtered, portMAX_DELAY) == pdTRUE) {
            anomaly_evaluate(&state, &filtered, &result);

            /* Update shared result under semaphore protection */
            if (xSemaphoreTake(xSemaphoreResult, pdMS_TO_TICKS(10)) == pdTRUE) {
                g_current_result = result;
                g_last_filtered.vibration_g   = filtered.vibration_g;
                g_last_filtered.temperature_c = filtered.temperature_c;
                g_last_filtered.timestamp_us  = filtered.timestamp_us;
                xSemaphoreGive(xSemaphoreResult);
            }
        }
    }
}

/* =========================================================
 * TASK: ALERT MANAGEMENT
 * Priority: TASK_PRIORITY_ALERT
 * Period:   100ms (for buzzer pulse timing)
 * ========================================================= */
static void task_alert(void *pvParameters) {
    ESP_LOGI(TAG, "Alert task started");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    system_health_t health = HEALTH_NORMAL;

    while (1) {
        /* Read current health (brief semaphore take) */
        if (xSemaphoreTake(xSemaphoreResult, pdMS_TO_TICKS(5)) == pdTRUE) {
            health = g_current_result.health;
            xSemaphoreGive(xSemaphoreResult);
        }
        alert_update(health);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

/* =========================================================
 * TASK: SERIAL COMMUNICATION
 * Priority: TASK_PRIORITY_COMM (lowest)
 * Period:   COMM_TRANSMIT_INTERVAL_MS (500ms)
 * ========================================================= */
static void task_comm(void *pvParameters) {
    ESP_LOGI(TAG, "Communication task started");
    TickType_t xLastWakeTime = xTaskGetTickCount();
    telemetry_packet_t pkt;

    while (1) {
        /* Snapshot current state safely */
        if (xSemaphoreTake(xSemaphoreResult, pdMS_TO_TICKS(10)) == pdTRUE) {
            pkt.device_id     = DEVICE_ID;
            pkt.vibration_g   = g_last_filtered.vibration_g;
            pkt.temperature_c = g_last_filtered.temperature_c;
            pkt.vib_severity  = g_current_result.vib_severity;
            pkt.temp_severity = g_current_result.temp_severity;
            pkt.health        = g_current_result.health;
            pkt.alert_count   = g_current_result.alert_count;
            pkt.timestamp_us  = g_last_filtered.timestamp_us;
            xSemaphoreGive(xSemaphoreResult);

            comm_transmit(&pkt);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(COMM_TRANSMIT_INTERVAL_MS));
    }
}

/* =========================================================
 * APPLICATION ENTRY POINT
 * ========================================================= */
void app_main(void) {
    ESP_LOGI(TAG, "=== Predictive Maintenance System v%s ===", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Device ID: %s", DEVICE_ID);

    /* Initialize NVS (required for WiFi/MQTT if enabled) */
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize hardware subsystems */
    ESP_ERROR_CHECK(sensors_init());
    ESP_ERROR_CHECK(alert_init());
    ESP_ERROR_CHECK(comm_init());

    /* Initialize shared result to safe defaults */
    g_current_result.health      = HEALTH_NORMAL;
    g_current_result.alert_count = 0;
    g_current_result.vib_severity  = 0.0f;
    g_current_result.temp_severity = 0.0f;

    /* Create inter-task communication primitives */
    xQueueSensorData = xQueueCreate(4, sizeof(sensor_data_t));
    xQueueFiltered   = xQueueCreate(4, sizeof(anomaly_input_t));
    xSemaphoreResult = xSemaphoreCreateMutex();

    configASSERT(xQueueSensorData != NULL);
    configASSERT(xQueueFiltered   != NULL);
    configASSERT(xSemaphoreResult != NULL);

    /* Create FreeRTOS tasks
     * Pin sensor & filter tasks to Core 1 for real-time determinism
     * Comm task on Core 0 (shared with WiFi stack if enabled)         */
    xTaskCreatePinnedToCore(task_sensor,  "sensor",  TASK_STACK_SENSOR,
                            NULL, TASK_PRIORITY_SENSOR,  NULL, 1);
    xTaskCreatePinnedToCore(task_filter,  "filter",  TASK_STACK_FILTER,
                            NULL, TASK_PRIORITY_FILTER,  NULL, 1);
    xTaskCreatePinnedToCore(task_anomaly, "anomaly", TASK_STACK_ANOMALY,
                            NULL, TASK_PRIORITY_ANOMALY, NULL, 1);
    xTaskCreatePinnedToCore(task_alert,   "alert",   TASK_STACK_ALERT,
                            NULL, TASK_PRIORITY_ALERT,   NULL, 0);
    xTaskCreatePinnedToCore(task_comm,    "comm",    TASK_STACK_COMM,
                            NULL, TASK_PRIORITY_COMM,    NULL, 0);

    ESP_LOGI(TAG, "All tasks created. System running.");
    /* app_main returns — FreeRTOS scheduler takes over */
}
