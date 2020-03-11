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
    char *ptr_l = (char*)sample_buff_ch0;
    char *ptr_r = (char*)sample_buff_ch1;
    uint8_t stride = sizeof(short);

    if (num_channels == 1) {
        ptr_r = ptr_l;
    }

    size_t bytes_written = 0;
    for (int i = 0; i < num_samples; i++) {
        /* low - high / low - high */
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]}; // ESP32 CPU is Little Endian
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, (const char *)&samp32, sizeof(samp32), &bytes_written, portMAX_DELAY);

        // DMA buffer full - retry
        if (bytes_written == 0) {
            i--;
        } else {
            ptr_l += stride;
            ptr_r += stride;
        }
    }
}

/* Called by the NXP modifications of libmad. Sets the needed output sample rate. */
void set_dac_sample_rate(int rate)
{
    i2s_output_set_sample_rate(rate);
}

static void audio_render_task(void *pvParameter)
{
    ESP_LOGI(TAG, "started.");

    while (1) {
        uint8_t *data = NULL;
        uint32_t size = 0;

        data = (uint8_t *)xRingbufferReceiveUpTo(audio_buff, &size, 1 / portTICK_RATE_MS, 512);

        if (data == NULL || size == 0) {
            continue;
        }

#if !defined(CONFIG_AUDIO_INPUT_NONE) || defined(CONFIG_ENABLE_AUDIO_PROMPT) || defined(CONFIG_ENABLE_VFX)
        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
#endif

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
        if (uxBits & AUDIO_PLAYER_RUN_BIT) {
            goto return_item;
        }
#endif

        set_dac_sample_rate(a2d_sample_rate);

        size_t bytes_written = 0;
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, data, size, &bytes_written, portMAX_DELAY);

#ifndef CONFIG_AUDIO_INPUT_NONE
        if (uxBits & AUDIO_INPUT_RUN_BIT) {
            goto return_item;
        }
#endif

#ifdef CONFIG_ENABLE_VFX
        if (!(uxBits & VFX_FFT_NULL_BIT)) {
            goto return_item;
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

return_item:
        vRingbufferReturnItem(audio_buff, (void *)data);
    }
}

void audio_render_init(void)
{
    memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));

    audio_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);
    if (!audio_buff) {
        ESP_LOGE(TAG, "failed to start audio render task");
    } else {
        xTaskCreatePinnedToCore(audio_render_task, "audioRenderT", 4096, NULL, configMAX_PRIORITIES - 3, NULL, 0);
    }
}
