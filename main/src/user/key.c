/*
 * key.c
 *
 *  Created on: 2018-05-31 14:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "core/os.h"
#include "user/key_handle.h"

#define TAG "key"

#ifdef CONFIG_ENABLE_SLEEP_KEY
static const uint8_t gpio_pin[] = {
#ifdef CONFIG_ENABLE_SLEEP_KEY
    CONFIG_SLEEP_KEY_PIN,
#endif
};

static const uint8_t gpio_val[] = {
#ifdef CONFIG_ENABLE_SLEEP_KEY
    #ifdef CONFIG_SLEEP_KEY_ACTIVE_LOW
        0,
    #else
        1,
    #endif
#endif
};

static const uint16_t gpio_hold[] = {
#ifdef CONFIG_ENABLE_SLEEP_KEY
    CONFIG_SLEEP_KEY_HOLD_TIME,
#endif
};

static void (*key_handle[])(void) = {
#ifdef CONFIG_ENABLE_SLEEP_KEY
    sleep_key_handle,
#endif
};

static void key_task(void *pvParameter)
{
    portTickType xLastWakeTime;
    uint16_t count[sizeof(gpio_pin)] = {0};

    for (int i=0; i<sizeof(gpio_pin); i++) {
        gpio_set_direction(gpio_pin[i], GPIO_MODE_INPUT);
        if (gpio_val[i] == 0) {
            gpio_pulldown_dis(gpio_pin[i]);
            gpio_pullup_en(gpio_pin[i]);
        } else {
            gpio_pullup_dis(gpio_pin[i]);
            gpio_pulldown_en(gpio_pin[i]);
        }
    }

    ESP_LOGI(TAG, "started.");

    vTaskDelay(2000 / portTICK_RATE_MS);

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            KEY_SCAN_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        xLastWakeTime = xTaskGetTickCount();

        for (int i=0; i<sizeof(gpio_pin); i++) {
            if (gpio_get_level(gpio_pin[i]) == gpio_val[i]) {
                if (++count[i] == gpio_hold[i] / 10) {
                    count[i] = 0;
                    key_handle[i]();
                }
            } else {
                count[i] = 0;
            }
        }

        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_RATE_MS);
    }
}

void key_init(void)
{
    xEventGroupSetBits(user_event_group, KEY_SCAN_RUN_BIT);

    xTaskCreatePinnedToCore(key_task, "keyT", 2048, NULL, 9, NULL, 1);
}
#endif
