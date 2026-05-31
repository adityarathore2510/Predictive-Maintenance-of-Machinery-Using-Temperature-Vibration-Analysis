/**
 * @file    alert.c
 * @brief   Alert Management Implementation — LED & Buzzer Control
 *
 * LEDC (LED Control) module is used for buzzer PWM:
 * - Timer runs at 5 kHz (audible center frequency for passive buzzers)
 * - Duty cycle 50% = maximum volume
 * - Duty cycle 0%  = silent
 *
 * For pulsed alerts, the alert_update() function is called periodically
 * by the alert FreeRTOS task, which toggles buzzer state at the
 * appropriate frequency.
 *
 * @author  Firmware Team — Agent 1
 */

#include "alert.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ALERT";

/* LEDC configuration */
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_SPEED_MODE     LEDC_LOW_SPEED_MODE
#define LEDC_FREQ_HZ        2000    /* 2kHz tone — audible without being harsh */
#define LEDC_RESOLUTION     LEDC_TIMER_8_BIT    /* 0–255 duty range */
#define LEDC_DUTY_50PCT     128                 /* 50% duty = max volume */

/* Internal state */
static bool s_buzzer_on    = false;
static bool s_initialized  = false;

/* =========================================================
 * PRIVATE HELPERS
 * ========================================================= */

static void set_led(int pin, bool on) {
    gpio_set_level(pin, on ? 1 : 0);
}

static void buzzer_on(void) {
    ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, LEDC_DUTY_50PCT);
    ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL);
    s_buzzer_on = true;
}

static void buzzer_off(void) {
    ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL);
    s_buzzer_on = false;
}

/* =========================================================
 * PUBLIC API
 * ========================================================= */

esp_err_t alert_init(void) {
    /* --- Configure LED GPIO pins as output --- */
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << PIN_LED_NORMAL)   |
                        (1ULL << PIN_LED_WARNING)   |
                        (1ULL << PIN_LED_CRITICAL),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));

    /* Default state: all LEDs off */
    set_led(PIN_LED_NORMAL,   false);
    set_led(PIN_LED_WARNING,  false);
    set_led(PIN_LED_CRITICAL, false);

    /* --- Configure LEDC Timer for Buzzer PWM --- */
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_SPEED_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_RESOLUTION,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    /* --- Configure LEDC Channel for Buzzer --- */
    ledc_channel_config_t ch_cfg = {
        .gpio_num   = PIN_BUZZER,
        .speed_mode = LEDC_SPEED_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,            /* Start silent */
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));

    s_initialized = true;
    ESP_LOGI(TAG, "Alert system initialized (LEDs: %d/%d/%d, Buzzer: GPIO%d)",
             PIN_LED_NORMAL, PIN_LED_WARNING, PIN_LED_CRITICAL, PIN_BUZZER);

    return ESP_OK;
}

void alert_update(system_health_t health) {
    if (!s_initialized) return;

    static uint32_t toggle_count = 0;
    toggle_count++;

    switch (health) {
        case HEALTH_NORMAL:
            set_led(PIN_LED_NORMAL,   true);
            set_led(PIN_LED_WARNING,  false);
            set_led(PIN_LED_CRITICAL, false);
            buzzer_off();
            break;

        case HEALTH_WARNING:
            set_led(PIN_LED_NORMAL,   false);
            set_led(PIN_LED_WARNING,  true);
            set_led(PIN_LED_CRITICAL, false);
            /* Slow pulse: toggle every ~500ms (task called at 100ms interval) */
            if (toggle_count % 5 == 0) {
                s_buzzer_on ? buzzer_off() : buzzer_on();
            }
            break;

        case HEALTH_CRITICAL:
            set_led(PIN_LED_NORMAL,   false);
            set_led(PIN_LED_WARNING,  false);
            set_led(PIN_LED_CRITICAL, true);
            /* Fast pulse: toggle every ~125ms */
            if (toggle_count % 2 == 0) {
                s_buzzer_on ? buzzer_off() : buzzer_on();
            }
            break;

        case HEALTH_FAULT:
        default:
            /* All LEDs flash on FAULT */
            if (toggle_count % 2 == 0) {
                set_led(PIN_LED_NORMAL,   true);
                set_led(PIN_LED_WARNING,  true);
                set_led(PIN_LED_CRITICAL, true);
                buzzer_on();
            } else {
                set_led(PIN_LED_NORMAL,   false);
                set_led(PIN_LED_WARNING,  false);
                set_led(PIN_LED_CRITICAL, false);
                buzzer_off();
            }
            break;
    }
}

void alert_silence(void) {
    buzzer_off();
    ESP_LOGI(TAG, "Alert silenced (manual override)");
}

void alert_deinit(void) {
    alert_silence();
    set_led(PIN_LED_NORMAL,   false);
    set_led(PIN_LED_WARNING,  false);
    set_led(PIN_LED_CRITICAL, false);
    s_initialized = false;
}
