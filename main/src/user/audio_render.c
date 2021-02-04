/*
 * audio_render.c
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#include "driver/i2s.h"

#include "core/os.h"
#include "chip/i2s.h"

#include "user/fft.h"
#include "user/bt_av.h"

#define TAG "audio_render"

RingbufHandle_t audio_buff = NULL;

static uint8_t buff_data[10 * 1024] = {0};
static StaticRingbuffer_t buff_struct = {0};

static void audio_render_task(void *pvParameter)
{
    uint16_t delay = 0;
    bool start = false;

    ESP_LOGI(TAG, "started.");

    while (1) {
        uint8_t *data = NULL;
        uint32_t size = 0;

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (!(uxBits & AUDIO_RENDER_CLR_BIT)) {
#ifdef CONFIG_ENABLE_VFX
            if (!(uxBits & AUDIO_INPUT_RUN_BIT) && (uxBits & AUDIO_INPUT_FFT_BIT)) {
                fft_init();
                xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);
            }
#endif
            xEventGroupSetBits(user_event_group, AUDIO_RENDER_CLR_BIT);
        }

        taskYIELD();

        if (start) {
            uint32_t remain = sizeof(buff_data) - xRingbufferGetCurFreeSize(audio_buff);

            if (remain >= FFT_BLOCK_SIZE) {
                delay = 0;

                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, portMAX_DELAY, FFT_BLOCK_SIZE);
            } else if (remain > 0) {
                delay = 0;

                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, portMAX_DELAY, remain);
            } else {
                if (++delay <= 256000 / a2d_sample_rate) {
                    vTaskDelay(1 / portTICK_RATE_MS);
                } else {
                    delay = 0;

                    xEventGroupClearBits(user_event_group, AUDIO_RENDER_CLR_BIT);

                    start = false;
                }

                continue;
            }
        } else {
            if (xRingbufferGetCurFreeSize(audio_buff) >= FFT_BLOCK_SIZE) {
                vTaskDelay(1 / portTICK_RATE_MS);
            } else {
                start = true;
            }

            continue;
        }

        uxBits = xEventGroupGetBits(user_event_group);
        if ((uxBits & AUDIO_PLAYER_RUN_BIT) || (uxBits & BT_A2DP_IDLE_BIT)
             || (uxBits & OS_PWR_SLEEP_BIT) || (uxBits & OS_PWR_RESET_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

        i2s_output_set_sample_rate(a2d_sample_rate);

        size_t bytes_written = 0;
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, data, size, &bytes_written, portMAX_DELAY);

#ifndef CONFIG_AUDIO_INPUT_NONE
        uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & AUDIO_INPUT_RUN_BIT) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }
#endif

#ifdef CONFIG_ENABLE_VFX
        uxBits = xEventGroupGetBits(user_event_group);
        if ((size < FFT_BLOCK_SIZE) || !(uxBits & VFX_FFT_IDLE_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

    #ifdef CONFIG_BT_AUDIO_FFT_ONLY_LEFT
        fft_load_data(data, FFT_CHANNEL_L);
    #elif defined(CONFIG_BT_AUDIO_FFT_ONLY_RIGHT)
        fft_load_data(data, FFT_CHANNEL_R);
    #else
        fft_load_data(data, FFT_CHANNEL_LR);
    #endif

        xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);
#endif

        vRingbufferReturnItem(audio_buff, (void *)data);
    }
}

void audio_render_init(void)
{
    audio_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);

    xTaskCreatePinnedToCore(audio_render_task, "audioRenderT", 1920, NULL, configMAX_PRIORITIES - 3, NULL, 0);
}
