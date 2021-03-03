/*
 * os.c
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"
#include "esp_sleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/rtc_io.h"

#include "core/os.h"
#include "user/audio_player.h"

#define OS_PWR_TAG "os_pwr"

EventGroupHandle_t user_event_group;

#if defined(CONFIG_ENABLE_SLEEP_KEY) || defined(CONFIG_ENABLE_BLE_CONTROL_IF)
static EventBits_t reset_wait_bits = OS_PWR_DUMMY_BIT;
static EventBits_t sleep_wait_bits = OS_PWR_DUMMY_BIT;

static void os_pwr_task_handle(void *pvParameters)
{
    ESP_LOGI(OS_PWR_TAG, "started.");

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            OS_PWR_RESET_BIT | OS_PWR_SLEEP_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & OS_PWR_RESET_BIT) {
            if (reset_wait_bits) {
                ESP_LOGW(OS_PWR_TAG, "waiting for unfinished jobs....");

                xEventGroupWaitBits(
                    user_event_group,
                    reset_wait_bits,
                    pdFALSE,
                    pdTRUE,
                    portMAX_DELAY
                );

                vTaskDelay(50 / portTICK_RATE_MS);
            }

            ESP_LOGW(OS_PWR_TAG, "reset now");
            esp_restart();
        } else if (uxBits & OS_PWR_SLEEP_BIT) {
            if (sleep_wait_bits) {
                ESP_LOGW(OS_PWR_TAG, "waiting for unfinished jobs....");

                xEventGroupWaitBits(
                    user_event_group,
                    sleep_wait_bits,
                    pdFALSE,
                    pdTRUE,
                    portMAX_DELAY
                );

                vTaskDelay(50 / portTICK_RATE_MS);
            }

#ifdef CONFIG_ENABLE_SLEEP_KEY
            esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
            rtc_gpio_set_direction(CONFIG_SLEEP_KEY_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    #ifdef CONFIG_SLEEP_KEY_ACTIVE_LOW
            rtc_gpio_pulldown_dis(CONFIG_SLEEP_KEY_PIN);
            rtc_gpio_pullup_en(CONFIG_SLEEP_KEY_PIN);
            esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_SLEEP_KEY_PIN, ESP_EXT1_WAKEUP_ALL_LOW);
    #else
            rtc_gpio_pullup_dis(CONFIG_SLEEP_KEY_PIN);
            rtc_gpio_pulldown_en(CONFIG_SLEEP_KEY_PIN);
            esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_SLEEP_KEY_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
    #endif
#endif

            ESP_LOGW(OS_PWR_TAG, "sleep now");
            esp_deep_sleep_start();
        }
    }
}

void os_pwr_reset_wait(EventBits_t bits)
{
    reset_wait_bits = bits;
    xEventGroupSetBits(user_event_group, OS_PWR_RESET_BIT);
}

void os_pwr_sleep_wait(EventBits_t bits)
{
    sleep_wait_bits = bits;
    xEventGroupSetBits(user_event_group, OS_PWR_SLEEP_BIT);
}
#endif

void os_init(void)
{
    user_event_group = xEventGroupCreate();

#if defined(CONFIG_ENABLE_SLEEP_KEY) || defined(CONFIG_ENABLE_BLE_CONTROL_IF)
    xTaskCreatePinnedToCore(os_pwr_task_handle, "osPwrT", 1280, NULL, 5, NULL, 0);
#endif

#ifdef CONFIG_ENABLE_SLEEP_KEY
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
#if CONFIG_SLEEP_KEY_HOLD_TIME > 250
        vTaskDelay((CONFIG_SLEEP_KEY_HOLD_TIME - 250) / portTICK_RATE_MS);

#ifdef CONFIG_SLEEP_KEY_ACTIVE_LOW
        if (rtc_gpio_get_level(CONFIG_SLEEP_KEY_PIN) == 0) {
#else
        if (rtc_gpio_get_level(CONFIG_SLEEP_KEY_PIN) == 1) {
#endif
            ESP_LOGW(OS_PWR_TAG, "resuming from sleep mode....");
        } else {
            ESP_LOGW(OS_PWR_TAG, "resuming aborted.");

            os_pwr_sleep_wait(OS_PWR_DUMMY_BIT);
        }
#else
        ESP_LOGW(OS_PWR_TAG, "resuming from sleep mode....");
#endif
    } else {
        os_pwr_sleep_wait(OS_PWR_DUMMY_BIT);
    }

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    audio_player_play_file(MP3_FILE_IDX_WAKEUP);
#endif
#endif
}
