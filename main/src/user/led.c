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
static const TickType_t led_mode_table[][2] = {
/*  { active, inactive }  */
    {   2000,     2000 },   // 0
    {   1000,     1000 },   // 1
    {    500,      500 },   // 2
    {    250,      250 },   // 3
    {    100,      100 },   // 4
    {     25,       25 },   // 5
    {    625,       25 },   // 6
    {   1250,       25 },   // 7
    {   1875,       25 },   // 8
    {   2500,       25 },   // 9
};

static uint8_t led_mode_index = 3;

static void led_task(void *pvParameter)
{
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

        vTaskDelayUntil(&xLastWakeTime, led_mode_table[led_mode_index][active] / portTICK_RATE_MS);
    }
}
#endif

void led_set_mode(uint8_t idx)
{
#ifdef CONFIG_ENABLE_LED
    if (idx >= sizeof(led_mode_table)/2) {
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
