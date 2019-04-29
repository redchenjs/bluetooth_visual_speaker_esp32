/*
 * audio_render.c
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#include "os/event.h"
#include "chip/i2s.h"
#include "user/fifo.h"

esp_err_t i2s_write_wrapper(i2s_port_t i2s_num, const void *src, size_t size, size_t *bytes_written, TickType_t ticks_to_wait)
{
    esp_err_t err = i2s_write(i2s_num, src, size, bytes_written, ticks_to_wait);

#ifdef CONFIG_ENABLE_VFX
        // Copy data to FIFO
        uint32_t idx = 0;
        const uint8_t *data = (const uint8_t *)src;
        while (size > 0) {
            fifo_write(data[idx+1] << 8 | data[idx]);
            idx  += 4;
            size -= 4;
        }
#endif

    return err;
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
    TickType_t max_wait = 20 / portTICK_PERIOD_MS; // portMAX_DELAY = bad idea
    for (int i = 0; i < num_samples; i++) {
        /* low - high / low - high */
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};
        i2s_write_wrapper(0, (const char *)&samp32, sizeof(samp32), &bytes_written, max_wait);

        // DMA buffer full - retry
        if (bytes_written == 0) {
            i--;
        } else {
            ptr_r += stride;
            ptr_l += stride;
        }
    }
}

/* Called by the NXP modifications of libmad. Sets the needed output sample rate. */
void set_dac_sample_rate(int rate)
{
    i2s0_set_sample_rate(rate);
}
