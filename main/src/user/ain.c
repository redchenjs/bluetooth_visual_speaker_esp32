/*
 * ain.c
 *
 *  Created on: 2019-07-05 21:22
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#include "core/os.h"
#include "core/app.h"

#include "chip/i2s.h"

#include "user/ain.h"
#include "user/fft.h"

#define TAG "ain"

static uint8_t data[FFT_BLOCK_SIZE] = {0};
static ain_mode_t ain_mode = DEFAULT_AIN_MODE;

static void ain_task(void *pvParameters)
{
    ESP_LOGI(TAG, "started.");

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            AUDIO_INPUT_RUN_BIT | AUDIO_INPUT_FFT_BIT | VFX_FFT_IDLE_BIT,
            pdFALSE,
            pdTRUE,
            portMAX_DELAY
        );

        size_t bytes_read = 0;
        i2s_read(CONFIG_AUDIO_INPUT_I2S_NUM, data, FFT_BLOCK_SIZE, &bytes_read, portMAX_DELAY);

#ifdef CONFIG_ENABLE_VFX
    #ifdef CONFIG_AUDIO_INPUT_FFT_ONLY_LEFT
        fft_load_data(data, FFT_CHANNEL_L);
    #elif defined(CONFIG_AUDIO_INPUT_FFT_ONLY_RIGHT)
        fft_load_data(data, FFT_CHANNEL_R);
    #else
        fft_load_data(data, FFT_CHANNEL_LR);
    #endif

        if (!(xEventGroupGetBits(user_event_group) & AUDIO_INPUT_RUN_BIT)) {
            fft_init();
        }

        xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);
#endif
    }
}

void ain_set_mode(ain_mode_t idx)
{
    ain_mode = idx;
    ESP_LOGI(TAG, "mode: %u", ain_mode);

    if (ain_mode == AIN_MODE_IDX_ON) {
        xEventGroupSetBits(user_event_group, AUDIO_INPUT_RUN_BIT);
    } else {
        xEventGroupClearBits(user_event_group, AUDIO_INPUT_RUN_BIT);
    }
}

ain_mode_t ain_get_mode(void)
{
    return ain_mode;
}

void ain_init(void)
{
    size_t length = sizeof(ain_mode_t);
    app_getenv("AIN_INIT_CFG", &ain_mode, &length);

    ain_set_mode(ain_mode);

    xTaskCreatePinnedToCore(ain_task, "ainT", 1536, NULL, configMAX_PRIORITIES - 3, NULL, 0);
}
