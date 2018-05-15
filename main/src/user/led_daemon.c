/*
 * led_daemon.c
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/led.h"

static const uint16_t led_mode_table[][2] = {/* { delay, count} */
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
static uint8_t led_mode_index = 7;

#define TAG "led"

void led_daemon(void *pvParameter)
{
    uint16_t i = 0;

    while (1) {
        portTickType xLastWakeTime = xTaskGetTickCount();
        if (i++ % led_mode_table[led_mode_index][1]) {
            led_off();
        } else {
            led_on();
        }
        vTaskDelayUntil(&xLastWakeTime, led_mode_table[led_mode_index][0]);
    }
}

void led_set_mode(uint8_t mode_index)
{
#if defined(CONFIG_ENABLE_LED)
    if (mode_index >= (sizeof(led_mode_table) / 2)) {
        ESP_LOGE(TAG, "invalid mode index");
        return;
    }
    led_mode_index = mode_index;
#endif
}
