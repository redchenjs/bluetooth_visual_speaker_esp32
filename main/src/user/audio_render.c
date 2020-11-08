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

RingbufHandle_t audio_buff = NULL;

static StaticRingbuffer_t buff_struct = {0};
static uint8_t buff_data[FFT_BLOCK_SIZE * 20] = {0};

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
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};

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

static void audio_buffer_reset(void)
{
    vTaskSuspendAll();

    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if (uxBits & BT_A2DP_DATA_BIT) {
        xTaskResumeAll();
        return;
    }

#ifdef CONFIG_ENABLE_VFX
    if (!(uxBits & AUDIO_INPUT_RUN_BIT) && (uxBits & AUDIO_INPUT_FFT_BIT)) {
        memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
        xEventGroupClearBits(user_event_group, VFX_FFT_NULL_BIT);
    }
#endif

    memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));
    audio_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);

    xTaskResumeAll();
}

static void audio_render_task(void *pvParameter)
{
    bool clear = false;
    bool start = false;
    uint16_t count = 0;
    EventBits_t uxBits = 0;

    ESP_LOGI(TAG, "started.");

    while (1) {
        uint8_t *data = NULL;
        uint32_t size = 0;
        uint32_t remain = 0;

        uxBits = xEventGroupGetBits(user_event_group);
        if (!clear && !(uxBits & AUDIO_RENDER_CLR_BIT)) {
            audio_buffer_reset();

            clear = true;

            xEventGroupSetBits(user_event_group, AUDIO_RENDER_CLR_BIT);
        }

        taskYIELD();

        if (start) {
            remain = sizeof(buff_data) - xRingbufferGetCurFreeSize(audio_buff);

            if (remain >= FFT_BLOCK_SIZE) {
                count = 0;

                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, 16 / portTICK_RATE_MS, FFT_BLOCK_SIZE);
            } else if (remain > 0 && !(remain % 4)) {
                count = 0;

                data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, 16 / portTICK_RATE_MS, remain);
            } else {
                if (++count < 200) {
                    vTaskDelay(1 / portTICK_RATE_MS);

                    continue;
                }

                clear = false;
                start = false;

                count = 0;

                continue;
            }

            if (data == NULL) {
                continue;
            }
        } else {
            if (xRingbufferGetCurFreeSize(audio_buff) < FFT_BLOCK_SIZE) {
                start = true;

                xEventGroupClearBits(user_event_group, AUDIO_RENDER_CLR_BIT);
            } else {
                vTaskDelay(1 / portTICK_RATE_MS);
            }

            continue;
        }

        uxBits = xEventGroupGetBits(user_event_group);
        if ((uxBits & AUDIO_PLAYER_RUN_BIT) || (uxBits & BT_A2DP_IDLE_BIT)
             || (uxBits & OS_PWR_SLEEP_BIT) || (uxBits & OS_PWR_RESET_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

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
        if ((size != FFT_BLOCK_SIZE) || !(uxBits & VFX_FFT_NULL_BIT)) {
            vRingbufferReturnItem(audio_buff, (void *)data);
            continue;
        }

        // copy data to FFT input buffer
        uint32_t idx = 0;

#ifdef CONFIG_BT_AUDIO_FFT_ONLY_LEFT
        int16_t data_l = 0;
        for (uint16_t k = 0; k < FFT_N; k++, idx += 4) {
            data_l = data[idx + 1] << 8 | data[idx];

            vfx_fft_input[k] = (float)data_l;
        }
#elif defined(CONFIG_BT_AUDIO_FFT_ONLY_RIGHT)
        int16_t data_r = 0;
        for (uint16_t k = 0; k < FFT_N; k++, idx += 4) {
            data_r = data[idx + 3] << 8 | data[idx + 2];

            vfx_fft_input[k] = (float)data_r;
        }
#else
        int16_t data_l = 0, data_r = 0;
        for (uint16_t k = 0; k < FFT_N; k++, idx += 4) {
            data_l = data[idx + 1] << 8 | data[idx];
            data_r = data[idx + 3] << 8 | data[idx + 2];

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
    memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));
    audio_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);

    xTaskCreatePinnedToCore(audio_render_task, "audioRenderT", 1920, NULL, configMAX_PRIORITIES - 3, NULL, 0);
}
