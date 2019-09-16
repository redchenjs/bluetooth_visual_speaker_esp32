/*
 * audio_input.c
 *
 *  Created on: 2019-07-05 21:22
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#include "core/os.h"
#include "chip/i2s.h"
#include "user/vfx.h"
#include "user/audio_input.h"

#define TAG "audio_input"

static uint8_t audio_input_mode = 1;

static TaskHandle_t audio_input_task_handle = NULL;

static void audio_input_task(void *pvParameters)
{
    size_t bytes_read = 0;
    char data[FFT_N * 4] = {0};

    ESP_LOGI(TAG, "started.");

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            AUDIO_INPUT_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        i2s_read(CONFIG_AUDIO_INPUT_I2S_NUM, data, FFT_N * 4, &bytes_read, portMAX_DELAY);

#ifdef CONFIG_ENABLE_VFX
        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & VFX_FFT_EXEC_BIT || uxBits & VFX_RELOAD_BIT) {
            continue;
        }

        // Copy data to FFT input buffer
        if (vfx_fft_plan) {
            uint32_t idx = 0;

#ifdef CONFIG_AUDIO_INPUT_FFT_ONLY_LEFT
            int16_t data_l = 0;
            for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
                data_l = data[idx+1] << 8 | data[idx];

                vfx_fft_plan->input[k] = (float)data_l;
            }
#elif defined(CONFIG_AUDIO_INPUT_FFT_ONLY_RIGHT)
            int16_t data_r = 0;
            for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
                data_r = data[idx+3] << 8 | data[idx+2];

                vfx_fft_plan->input[k] = (float)data_r;
            }
#else
            int16_t data_l = 0, data_r = 0;
            for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
                data_l = data[idx+1] << 8 | data[idx];
                data_r = data[idx+3] << 8 | data[idx+2];

                vfx_fft_plan->input[k] = (float)((data_l + data_r) / 2);
            }
#endif

            xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
        }
#endif // CONFIG_ENABLE_VFX
    }
}

void audio_input_set_mode(uint8_t idx)
{
#ifndef CONFIG_AUDIO_INPUT_NONE
    audio_input_mode = idx;
    ESP_LOGI(TAG, "mode %u", audio_input_mode);

    if (audio_input_mode) {
        if (!audio_input_task_handle) {
            i2s_input_init();
            audio_input_init();
        }
    } else {
        if (audio_input_task_handle) {
            audio_input_deinit();
            i2s_input_deinit();
        }
    }

    xEventGroupSetBits(user_event_group, VFX_RELOAD_BIT | VFX_FFT_FULL_BIT);
#endif
}

uint8_t audio_input_get_mode(void)
{
    return audio_input_mode;
}

void audio_input_init(void)
{
    xEventGroupSetBits(user_event_group, AUDIO_INPUT_RUN_BIT);

    xTaskCreatePinnedToCore(audio_input_task, "AudioInputT", 2048, NULL, 8, &audio_input_task_handle, 1);
}

void audio_input_deinit(void)
{
    xEventGroupClearBits(user_event_group, AUDIO_INPUT_RUN_BIT);

    xEventGroupWaitBits(
        user_event_group,
        VFX_FFT_FULL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    vTaskDelete(audio_input_task_handle);
    audio_input_task_handle = NULL;

    ESP_LOGI(TAG, "stopped.");
}
