/*
 * audio_daemon.c
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

#include "system/event.h"
#include "user/audio_daemon.h"

#define TAG "audio"

static const uint8_t *mp3_file_ptr[][2] = {
                                            {snd0_mp3_ptr, snd0_mp3_end},
                                            {snd1_mp3_ptr, snd1_mp3_end}
                                        };
static uint8_t mp3_file_index = 0;

void audio_daemon(void *pvParameters)
{
    // allocate structs needed for mp3 decoding
    struct mad_stream *stream = malloc(sizeof(struct mad_stream));
    struct mad_frame  *frame  = malloc(sizeof(struct mad_frame));
    struct mad_synth  *synth  = malloc(sizeof(struct mad_synth));

    if (stream == NULL) {ESP_LOGE(TAG, "malloc(stream) failed"); goto err;}
    if (frame  == NULL) {ESP_LOGE(TAG, "malloc(frame) failed");  goto err;}
    if (synth  == NULL) {ESP_LOGE(TAG, "malloc(synth) failed");  goto err;}

    while (1) {
        xEventGroupWaitBits(
            daemon_event_group,
            AUDIO_DAEMON_READY_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        // initialize mp3 parts
        mad_stream_init(stream);
        mad_frame_init(frame);
        mad_synth_init(synth);

        mad_stream_buffer(
            stream,
            mp3_file_ptr[mp3_file_index][0],
            mp3_file_ptr[mp3_file_index][1] - mp3_file_ptr[mp3_file_index][0]
        );
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

        mad_synth_finish(synth);
        mad_frame_finish(frame);
        mad_stream_finish(stream);
    }
err:
    free(synth);
    free(frame);
    free(stream);
    ESP_LOGE(TAG, "task failed, rebooting...");
    esp_restart();
}

void audio_play_file(uint8_t filename_index)
{
    if (filename_index >= (sizeof(mp3_file_ptr) / 2)) {
        ESP_LOGE(TAG, "invalid filename index");
        return;
    }
    mp3_file_index = filename_index;
    xEventGroupSetBits(daemon_event_group, AUDIO_DAEMON_READY_BIT);
}
