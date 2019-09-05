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

#include "core/os.h"
#include "user/audio_mp3.h"

#define TAG "os"

EventGroupHandle_t user_event_group;

#if defined(CONFIG_ENABLE_WAKEUP_KEY) || defined(CONFIG_ENABLE_SLEEP_KEY)
void os_enter_sleep_mode(void)
{
    ESP_LOGI(TAG, "entering sleep mode");

#ifdef CONFIG_ENABLE_WAKEUP_KEY
    #ifdef CONFIG_WAKEUP_KEY_ACTIVE_LOW
        esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_WAKEUP_KEY_PIN, ESP_EXT1_WAKEUP_ALL_LOW);
    #else
        esp_sleep_enable_ext1_wakeup(1ULL << CONFIG_WAKEUP_KEY_PIN, ESP_EXT1_WAKEUP_ANY_HIGH);
    #endif // #ifdef CONFIG_WAKEUP_KEY_ACTIVE_LOW
#endif // #ifdef CONFIG_ENABLE_WAKEUP_KEY

    esp_deep_sleep_start();
}
#endif // #if defined(CONFIG_ENABLE_WAKEUP_KEY) || defined(CONFIG_ENABLE_SLEEP_KEY)

void os_init(void)
{
#ifdef CONFIG_ENABLE_WAKEUP_KEY
    ESP_LOGI(TAG, "checking wakeup cause");
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
        vTaskDelay(CONFIG_WAKEUP_KEY_EXTRA_HOLD_TIME / portTICK_RATE_MS);

#ifdef CONFIG_WAKEUP_KEY_ACTIVE_LOW
        if (gpio_get_level(CONFIG_WAKEUP_KEY_PIN) == 0) {
#else
        if (gpio_get_level(CONFIG_WAKEUP_KEY_PIN) == 1) {
#endif
            ESP_LOGI(TAG, "resuming from sleep mode");
        } else {
            ESP_LOGI(TAG, "resume aborted");

            os_enter_sleep_mode();
        }
    } else {
        os_enter_sleep_mode();
    }
#endif // CONFIG_ENABLE_WAKEUP_KEY

    user_event_group = xEventGroupCreate();

#ifdef CONFIG_ENABLE_WAKEUP_KEY
    audio_mp3_play(2);
#endif
}
