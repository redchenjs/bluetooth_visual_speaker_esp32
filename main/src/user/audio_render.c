/*
 * audio_render.c
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#include "os/core.h"
#include "chip/i2s.h"
#include "user/vfx_core.h"

void i2s_write_wrapper(const void *src, size_t size, int bits, int flag)
{
    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if ( !(uxBits & AUDIO_INPUT_RUN_BIT) ) {
        size_t bytes_written;
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, src, size, &bytes_written, portMAX_DELAY);
    }

#ifdef CONFIG_ENABLE_VFX
    if (flag) {
        // Copy data to FIFO
        uint32_t idx = 0;
        const uint8_t *data = (const uint8_t *)src;
        while (size > 0) {
            int16_t data_l = data[idx+1] << 8 | data[idx];
            int16_t data_r = data[idx+3] << 8 | data[idx+2];
            vfx_fifo_write((data_l + data_r) / 2);
            idx  += 4;
            size -= 4;
        }
    }
#endif
}

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
        i2s_write_wrapper((const char *)&samp32, sizeof(samp32), 16, 0);

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
    i2s_set_output_sample_rate(rate);
}
