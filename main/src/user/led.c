/*
 * led.c
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "user/led.h"

#define TAG "led"

static const TickType_t led_mode_table[][2] = {
    [LED_MODE_IDX_BLINK_S1] = {2000, 2000},
    [LED_MODE_IDX_BLINK_S0] = {1000, 1000},
    [LED_MODE_IDX_BLINK_M1] = { 500,  500},
    [LED_MODE_IDX_BLINK_M0] = { 250,  250},
    [LED_MODE_IDX_BLINK_F1] = { 100,  100},
    [LED_MODE_IDX_BLINK_F0] = {  50,   50},
    [LED_MODE_IDX_PULSE_D0] = { 625,   25},
    [LED_MODE_IDX_PULSE_D1] = {1250,   25},
    [LED_MODE_IDX_PULSE_D2] = {1875,   25},
    [LED_MODE_IDX_PULSE_D3] = {2500,   25}
};

static led_mode_t led_mode = LED_MODE_IDX_BLINK_M0;

static void led_task(void *pvParameter)
{
#ifdef CONFIG_ENABLE_LED
    bool active = false;
    portTickType xLastWakeTime;

#ifdef CONFIG_LED_ACTIVE_LOW
    gpio_set_level(CONFIG_LED_PIN, 1);
#else
    gpio_set_level(CONFIG_LED_PIN, 0);
#endif

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(CONFIG_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "started.");

    while (1) {
        xLastWakeTime = xTaskGetTickCount();

#ifdef CONFIG_LED_ACTIVE_LOW
        if (!active) {
#else
        if (active) {
#endif
            gpio_set_level(CONFIG_LED_PIN, 1);
        } else {
            gpio_set_level(CONFIG_LED_PIN, 0);
        }

        active = !active;

        vTaskDelayUntil(&xLastWakeTime, led_mode_table[led_mode][active] / portTICK_RATE_MS);
    }
#endif
}

void led_set_mode(led_mode_t idx)
{
    if (idx >= sizeof(led_mode_table) / sizeof(led_mode_table[0])) {
        return;
    }

    led_mode = idx;
}

led_mode_t led_get_mode(void)
{
    return led_mode;
}

void led_init(void)
{
    xTaskCreatePinnedToCore(led_task, "ledT", 1536, NULL, 9, NULL, 1);
}
