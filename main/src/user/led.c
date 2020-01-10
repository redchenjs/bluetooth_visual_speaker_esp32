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

#define TAG "led"

#ifdef CONFIG_ENABLE_LED
static const uint16_t led_mode_table[][2] = {
/*  { delay, count}  */
    {     0,     2},   // 0, Keep off
    {  1000,     2},   // 1,
    {   500,     2},   // 2,
    {   250,     2},   // 3,
    {   100,     2},   // 4,
    {    25,     2},   // 5,
    {    25,    25},   // 6,
    {    25,    50},   // 7,
    {    25,   100},   // 8,
    {    25,     0}    // 9, Keep on
};

static uint8_t led_mode_index = 3;

static void led_task(void *pvParameter)
{
    uint16_t i = 0;
    portTickType xLastWakeTime;

    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "started.");

    while (1) {
        xLastWakeTime = xTaskGetTickCount();

        if (i++ % led_mode_table[led_mode_index][1]) {
#ifdef CONFIG_LED_ACTIVE_LOW
            gpio_set_level(CONFIG_LED_PIN, 1);
        } else {
            gpio_set_level(CONFIG_LED_PIN, 0);
        }
#else
            gpio_set_level(CONFIG_LED_PIN, 0);
        } else {
            gpio_set_level(CONFIG_LED_PIN, 1);
        }
#endif

        vTaskDelayUntil(&xLastWakeTime, led_mode_table[led_mode_index][0] / portTICK_RATE_MS);
    }
}
#endif

void led_set_mode(uint8_t idx)
{
#ifdef CONFIG_ENABLE_LED
    if (idx >= (sizeof(led_mode_table) / 2)) {
        ESP_LOGE(TAG, "invalid mode index");
        return;
    }
    led_mode_index = idx;
#endif
}

#ifdef CONFIG_ENABLE_LED
void led_init(void)
{
    xTaskCreatePinnedToCore(led_task, "ledT", 1280, NULL, 9, NULL, 1);
}
#endif
