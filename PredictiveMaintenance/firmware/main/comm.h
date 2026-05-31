/**
 * @file    comm.h
 * @brief   Communication Module API — Serial (UART) + Optional MQTT
 *
 * WHY THIS MODULE EXISTS:
 * This module decouples the data transmission strategy from the rest
 * of the application. Currently supports UART serial (primary) and
 * MQTT over WiFi (optional compile-time feature).
 *
 * OUTPUT FORMAT:
 * JSON over serial, one packet per transmission interval.
 * Example packet:
 * {
 *   "id":    "ESP32-PM-01",
 *   "ts":    1234567890,
 *   "vib":   0.342,
 *   "temp":  42.1,
 *   "vs":    0.17,
 *   "ts_s":  0.53,
 *   "health":"NORMAL",
 *   "alerts":0
 * }
 *
 * WHY JSON:
 * JSON is human-readable, self-describing, and supported natively
 * by the Python dashboard's json.loads(). For bandwidth-constrained
 * environments, this can be replaced with binary framing (e.g., COBS).
 *
 * @author  Firmware Team — Agent 1
 */

#ifndef COMM_H
#define COMM_H

#include "anomaly.h"
#include "sensors.h"
#include "esp_err.h"

/* =========================================================
 * DATA STRUCTURES
 * ========================================================= */

/**
 * @brief Telemetry packet — full snapshot of machine state
 */
typedef struct {
    const char      *device_id;
    float            vibration_g;
    float            temperature_c;
    float            vib_severity;
    float            temp_severity;
    system_health_t  health;
    uint32_t         alert_count;
    int64_t          timestamp_us;
} telemetry_packet_t;

/* =========================================================
 * PUBLIC API
 * ========================================================= */

/**
 * @brief Initialize UART serial port for dashboard communication
 *
 * UART0 is shared with IDF console — UART2 is used here to avoid conflicts.
 * Baud rate: 115200, 8N1 format.
 *
 * @return ESP_OK on success
 */
esp_err_t comm_init(void);

/**
 * @brief Transmit a telemetry packet as JSON over serial
 *
 * @param[in] packet  Pointer to populated telemetry packet
 * @return ESP_OK on success
 */
esp_err_t comm_transmit(const telemetry_packet_t *packet);

/**
 * @brief Deinitialize communication hardware
 */
void comm_deinit(void);

#endif /* COMM_H */
