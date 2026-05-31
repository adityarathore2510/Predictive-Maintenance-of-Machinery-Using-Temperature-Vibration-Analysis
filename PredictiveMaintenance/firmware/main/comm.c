/**
 * @file    comm.c
 * @brief   Communication Implementation — UART Serial Output
 *
 * Formats telemetry data as compact JSON and transmits over UART2.
 * The Python dashboard reads this stream and parses each JSON line.
 *
 * PROTOCOL: One JSON object per line, newline-terminated.
 * This enables line-by-line parsing in Python with readline().
 *
 * NOTE: snprintf is used (not sprintf) to prevent buffer overflows —
 * a critical security practice even in embedded contexts.
 *
 * @author  Firmware Team — Agent 1
 */

#include "comm.h"
#include "config.h"
#include "anomaly.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "COMM";

/* UART configuration */
#define COMM_UART_PORT      UART_NUM_2
#define COMM_TX_PIN         17      /* GPIO17 — UART2 TX */
#define COMM_RX_PIN         16      /* GPIO16 — UART2 RX (unused for output) */
#define COMM_TX_BUF_SIZE    512
#define COMM_JSON_BUF_SIZE  256

/* =========================================================
 * PUBLIC API
 * ========================================================= */

esp_err_t comm_init(void) {
    const uart_config_t uart_cfg = {
        .baud_rate  = SERIAL_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(COMM_UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(COMM_UART_PORT, COMM_TX_PIN, COMM_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(COMM_UART_PORT,
                                        COMM_TX_BUF_SIZE, COMM_TX_BUF_SIZE,
                                        0, NULL, 0));

    ESP_LOGI(TAG, "UART2 initialized at %d baud (TX: GPIO%d)",
             SERIAL_BAUD_RATE, COMM_TX_PIN);
    return ESP_OK;
}

esp_err_t comm_transmit(const telemetry_packet_t *packet) {
    if (packet == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char json_buf[COMM_JSON_BUF_SIZE];

    /* Format compact JSON — all fields on one line, newline-terminated */
    int len = snprintf(json_buf, sizeof(json_buf),
        "{\"id\":\"%s\",\"ts\":%lld"
        ",\"vib\":%.4f,\"temp\":%.2f"
        ",\"vs\":%.3f,\"ts_s\":%.3f"
        ",\"health\":\"%s\",\"alerts\":%lu}\n",
        packet->device_id,
        (long long)(packet->timestamp_us / 1000000LL),   /* seconds since boot */
        packet->vibration_g,
        packet->temperature_c,
        packet->vib_severity,
        packet->temp_severity,
        anomaly_health_to_string(packet->health),
        (unsigned long)packet->alert_count
    );

    if (len <= 0 || len >= (int)sizeof(json_buf)) {
        ESP_LOGE(TAG, "JSON buffer overflow or format error");
        return ESP_FAIL;
    }

    /* Write to UART (non-blocking, uses internal driver buffer) */
    int written = uart_write_bytes(COMM_UART_PORT, json_buf, (size_t)len);
    if (written < 0) {
        ESP_LOGE(TAG, "UART write failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void comm_deinit(void) {
    uart_driver_delete(COMM_UART_PORT);
    ESP_LOGI(TAG, "UART deinitialized");
}
