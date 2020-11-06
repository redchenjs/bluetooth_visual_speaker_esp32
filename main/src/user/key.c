/*
 * key.c
 *
 *  Created on: 2018-05-31 14:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "core/os.h"
#include "user/key.h"
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
#endif

static key_scan_mode_t key_scan_mode = KEY_SCAN_MODE_IDX_OFF;

static void key_task(void *pvParameter)
{
#ifdef CONFIG_ENABLE_SLEEP_KEY
    portTickType xLastWakeTime;
    uint16_t count[sizeof(gpio_pin)] = {0};

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE
    };

    for (int i = 0; i < sizeof(gpio_pin); i++) {
        io_conf.pin_bit_mask = BIT64(gpio_pin[i]);

        if (gpio_val[i] == 0) {
            io_conf.pull_up_en = true;
            io_conf.pull_down_en = false;
        } else {
            io_conf.pull_up_en = false;
            io_conf.pull_down_en = true;
        }

        gpio_config(&io_conf);
    }

    ESP_LOGI(TAG, "started.");

    vTaskDelay(2000 / portTICK_RATE_MS);

    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(
            user_event_group,
            KEY_SCAN_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        if (uxBits & KEY_SCAN_CLR_BIT) {
            memset(&count, 0x00, sizeof(count));

            xEventGroupClearBits(user_event_group, KEY_SCAN_CLR_BIT);
        }

        xLastWakeTime = xTaskGetTickCount();

        for (int i = 0; i < sizeof(gpio_pin); i++) {
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
#endif
}

void key_set_scan_mode(key_scan_mode_t idx)
{
    key_scan_mode = idx;

    if (key_scan_mode == KEY_SCAN_MODE_IDX_ON) {
        xEventGroupSetBits(user_event_group, KEY_SCAN_RUN_BIT | KEY_SCAN_CLR_BIT);
    } else {
        xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);
    }
}

key_scan_mode_t key_get_scan_mode(void)
{
    return key_scan_mode;
}

void key_init(void)
{
    key_set_scan_mode(KEY_SCAN_MODE_IDX_ON);

    xTaskCreatePinnedToCore(key_task, "keyT", 1536, NULL, 9, NULL, 1);
}
