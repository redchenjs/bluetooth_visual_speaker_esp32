/*
 * audio_render.c
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"

#include "driver/i2s.h"

#include "core/os.h"
#include "chip/i2s.h"
#include "user/vfx.h"
#include "user/bt_av.h"

#define TAG "audio_render"

static uint8_t buff_data[8*1024] = {0};
static StaticRingbuffer_t buff_struct = {0};

RingbufHandle_t audio_buff = NULL;

/* render callback for the libmad synth */
void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{
    // pointer to left / right sample position
    char *ptr_l = (char *)sample_buff_ch0;
    char *ptr_r = (char *)sample_buff_ch1;
    uint8_t stride = sizeof(short);

    if (num_channels == 1) {
        ptr_r = ptr_l;
    }

    size_t bytes_written = 0;
    for (int i = 0; i < num_samples; i++) {
        /* low - high / low - high */
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]}; // ESP32 CPU is little-endian

        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, (const char *)&samp32, sizeof(samp32), &bytes_written, portMAX_DELAY);

        ptr_l += stride;
        ptr_r += stride;
    }
}

/* frame callback for the libmad synth, set the needed output sample rate */
void set_dac_sample_rate(int rate)
{
    i2s_output_set_sample_rate(rate);
}

static void audio_render_task(void *pvParameter)
{
    bool start = false;

    ESP_LOGI(TAG, "started.");

    while (1) {
        uint8_t *data = NULL;
        uint32_t size = 0;
        uint32_t remain = 0;

        xEventGroupWaitBits(
            user_event_group,
            AUDIO_RENDER_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        if (start) {
            remain = sizeof(buff_data) - xRingbufferGetCurFreeSize(audio_buff);

            if (remain >= 512) {
                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, 16 / portTICK_RATE_MS, 512);
            } else if (remain > 0) {
                if ((remain % 4) != 0) {
                    taskYIELD();
                    continue;
                }

                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, 16 / portTICK_RATE_MS, remain);
            } else {
                taskYIELD();

                remain = sizeof(buff_data) - xRingbufferGetCurFreeSize(audio_buff);
                if (remain == 0) {
#ifdef CONFIG_ENABLE_VFX
                    uxBits = xEventGroupGetBits(user_event_group);
                    if (!(uxBits & AUDIO_INPUT_RUN_BIT) && (uxBits & AUDIO_INPUT_FFT_BIT)) {
                        memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
                        xEventGroupClearBits(user_event_group, VFX_FFT_NULL_BIT);
                    }
#endif
                    start = false;
                }

                continue;
            }

            if (data == NULL) {
                taskYIELD();
                continue;
            }
        } else {
            vTaskDelay(1 / portTICK_RATE_MS);

            if (xRingbufferGetCurFreeSize(audio_buff) == 0) {
                start = true;
            }

            continue;
        }

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);

        if ((uxBits & BT_A2DP_IDLE_BIT) || (uxBits & OS_PWR_SLEEP_BIT) || (uxBits & OS_PWR_RESTART_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
        if (!(uxBits & AUDIO_PLAYER_IDLE_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }
#endif

        set_dac_sample_rate(a2d_sample_rate);

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
        if ((size != 512) || !(uxBits & VFX_FFT_NULL_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

        // Copy data to FFT input buffer
        uint32_t idx = 0;

#ifdef CONFIG_BT_AUDIO_FFT_ONLY_LEFT
        int16_t data_l = 0;
        for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
            data_l = data[idx+1] << 8 | data[idx];

            vfx_fft_input[k] = (float)data_l;
        }
#elif defined(CONFIG_BT_AUDIO_FFT_ONLY_RIGHT)
        int16_t data_r = 0;
        for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
            data_r = data[idx+3] << 8 | data[idx+2];

            vfx_fft_input[k] = (float)data_r;
        }
#else
        int16_t data_l = 0, data_r = 0;
        for (uint16_t k=0; k<FFT_N; k++,idx+=4) {
            data_l = data[idx+1] << 8 | data[idx];
            data_r = data[idx+3] << 8 | data[idx+2];

            vfx_fft_input[k] = (float)((data_l + data_r) / 2);
        }
#endif

        xEventGroupClearBits(user_event_group, VFX_FFT_NULL_BIT);
#endif

        vRingbufferReturnItem(audio_buff, (void *)data);
    }
}

void audio_render_init(void)
{
    xEventGroupSetBits(user_event_group, AUDIO_RENDER_RUN_BIT);

    memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));

    audio_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);
    if (!audio_buff) {
        ESP_LOGE(TAG, "failed to start audio render task");
    } else {
        xTaskCreatePinnedToCore(audio_render_task, "audioRenderT", 2048, NULL, configMAX_PRIORITIES - 3, NULL, 0);
    }
}
