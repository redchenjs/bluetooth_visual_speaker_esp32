/*
 * mp3_player.c
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "mad.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "driver/i2s.h"

#include "tasks/main_task.h"
#include "tasks/mp3_player.h"

#define TAG "mp3_player"

static const uint8_t *mp3_file_ptr[][2] = {
                                            {snd0_mp3_ptr, snd0_mp3_end}, // 建立连接提示音
                                            {snd1_mp3_ptr, snd1_mp3_end}  // 断开连接提示音
                                        };
uint8_t mp3_file_index = 0;

void mp3_player_play_file(uint8_t filename_index)
{
    if (filename_index >= (sizeof(mp3_file_ptr) / 2)) {
        ESP_LOGE(TAG, "invalid filename index");
        return;
    }
    mp3_file_index = filename_index;
    xEventGroupSetBits(task_event_group, MP3_PLAYER_READY_BIT);
}

void mp3_player_task(void *pvParameters)
{
    //Allocate structs needed for mp3 decoding
    struct mad_stream *stream = malloc(sizeof(struct mad_stream));
    struct mad_frame  *frame  = malloc(sizeof(struct mad_frame));
    struct mad_synth  *synth  = malloc(sizeof(struct mad_synth));

    if (stream == NULL) { ESP_LOGE(TAG, "malloc(stream) failed"); goto err; }
    if (frame  == NULL) { ESP_LOGE(TAG, "malloc(frame) failed");  goto err; }
    if (synth  == NULL) { ESP_LOGE(TAG, "malloc(synth) failed");  goto err; }

    //Initialize mp3 parts
    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);

    while (1) {
        xEventGroupWaitBits(task_event_group, MP3_PLAYER_READY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

        mad_stream_buffer(stream, mp3_file_ptr[mp3_file_index][0], mp3_file_ptr[mp3_file_index][1] - mp3_file_ptr[mp3_file_index][0]);
        while (1) {
            if (mad_frame_decode(frame, stream) == -1) {
                if (!MAD_RECOVERABLE(stream->error)) {
                    break;
                }
                ESP_LOGE(TAG, "dec err 0x%04x (%s)", stream->error, mad_stream_errorstr(stream));
                continue;
            }
            mad_synth_frame(synth, frame);
        }
        // avoid noise
        i2s_zero_dma_buffer(0);
    }

    mad_synth_finish(synth);
    mad_frame_finish(frame);
    mad_stream_finish(stream);
err:
    free(synth);
    free(frame);
    free(stream);
    ESP_LOGE(TAG, "task failed, rebooting...");
    esp_restart();
}

/* render callback for the libmad synth */
void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{
    // pointer to left / right sample position
    char *ptr_l = (char*) sample_buff_ch0;
    char *ptr_r = (char*) sample_buff_ch1;
    uint8_t stride = sizeof(short);

    if (num_channels == 1) {
        ptr_r = ptr_l;
    }

    int bytes_pushed = 0;
    TickType_t max_wait = 20 / portTICK_PERIOD_MS; // portMAX_DELAY = bad idea
    for (int i = 0; i < num_samples; i++) {
        /* low - high / low - high */
        const char samp32[4] = {ptr_l[0], ptr_l[1], ptr_r[0], ptr_r[1]};
        bytes_pushed = i2s_push_sample(0, (const char*) &samp32, max_wait);

        // DMA buffer full - retry
        if (bytes_pushed == 0) {
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
    static int dac_sample_rate = 44100;
    if (rate != dac_sample_rate) {
        i2s_set_sample_rates(0, rate);
        dac_sample_rate = rate;
    }
}
