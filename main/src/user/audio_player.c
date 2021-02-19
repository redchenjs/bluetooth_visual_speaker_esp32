/*
 * audio_player.c
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s.h"

#include "mad.h"

#include "frame.h"
#include "synth.h"
#include "stream.h"

#include "core/os.h"
#include "chip/i2s.h"

#include "user/audio_player.h"

#define TAG "audio_player"

static const char *mp3_file_ptr[][2] = {
#ifdef CONFIG_AUDIO_PROMPT_CONNECTED
    [MP3_FILE_IDX_CONNECTED] = {snd0_mp3_ptr, snd0_mp3_end},
#else
    [MP3_FILE_IDX_CONNECTED] = {NULL, NULL},
#endif
#ifdef CONFIG_AUDIO_PROMPT_DISCONNECTED
    [MP3_FILE_IDX_DISCONNECTED] = {snd1_mp3_ptr, snd1_mp3_end},
#else
    [MP3_FILE_IDX_DISCONNECTED] = {NULL, NULL},
#endif
#ifdef CONFIG_AUDIO_PROMPT_SLEEP
    [MP3_FILE_IDX_SLEEP] = {snd2_mp3_ptr, snd2_mp3_end},
#else
    [MP3_FILE_IDX_SLEEP] = {NULL, NULL},
#endif
#ifdef CONFIG_AUDIO_PROMPT_WAKEUP
    [MP3_FILE_IDX_WAKEUP] = {snd3_mp3_ptr, snd3_mp3_end},
#else
    [MP3_FILE_IDX_WAKEUP] = {NULL, NULL},
#endif
    [MP3_FILE_IDX_MAX] = {NULL, NULL}
};

static bool playback_pending = false;
static mp3_file_t mp3_file = MP3_FILE_IDX_MAX;

/* render callback for the libmad synth */
void render_sample_block(short *sample_ch0, short *sample_ch1, unsigned int sample_rate, unsigned int nch, unsigned int ns)
{
    if (nch == 1) {
        sample_ch1 = sample_ch0;
    }

    i2s_output_set_sample_rate(sample_rate);

    size_t bytes_written = 0;
    for (int i = 0; i < ns; i++) {
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, sample_ch0++, sizeof(short), &bytes_written, portMAX_DELAY);
        i2s_write(CONFIG_AUDIO_OUTPUT_I2S_NUM, sample_ch1++, sizeof(short), &bytes_written, portMAX_DELAY);
    }
}

static void audio_player_task(void *pvParameters)
{
    ESP_LOGI(TAG, "started.");

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            AUDIO_PLAYER_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        // allocate structs needed for mp3 decoding
        struct mad_stream *stream = malloc(sizeof(struct mad_stream));
        struct mad_frame  *frame  = malloc(sizeof(struct mad_frame));
        struct mad_synth  *synth  = malloc(sizeof(struct mad_synth));

        if ((stream == NULL) || (frame == NULL) || (synth == NULL)) {
            xEventGroupSetBits(user_event_group, AUDIO_PLAYER_IDLE_BIT);
            xEventGroupClearBits(user_event_group, AUDIO_PLAYER_RUN_BIT);

            ESP_LOGE(TAG, "failed to allocate memory");

            playback_pending = false;

            free(synth);
            free(frame);
            free(stream);

            continue;
        }

        // initialize mp3 parts
        mad_stream_init(stream);
        mad_frame_init(frame);
        mad_synth_init(synth);

        mad_stream_buffer(stream, (const unsigned char *)mp3_file_ptr[mp3_file][0],
                          mp3_file_ptr[mp3_file][1] - mp3_file_ptr[mp3_file][0]);

        while (1) {
            if (mad_frame_decode(frame, stream) == -1) {
                if (MAD_RECOVERABLE(stream->error)) {
                    ESP_LOGE(TAG, "dec err 0x%04x (%s)", stream->error, mad_stream_errorstr(stream));

                    continue;
                }

                break;
            }

            mad_synth_frame(synth, frame);
        }

        mad_synth_finish(synth);
        mad_frame_finish(frame);
        mad_stream_finish(stream);

        free(synth);
        free(frame);
        free(stream);

        if (playback_pending) {
            playback_pending = false;
        } else {
            xEventGroupSetBits(user_event_group, AUDIO_PLAYER_IDLE_BIT);
            xEventGroupClearBits(user_event_group, AUDIO_PLAYER_RUN_BIT);
        }
    }
}

void audio_player_play_file(mp3_file_t idx)
{
    if (idx >= sizeof(mp3_file_ptr) / sizeof(mp3_file_ptr[0])) {
        return;
    }

    if (mp3_file_ptr[idx][0] == NULL || mp3_file_ptr[idx][1] == NULL) {
        return;
    }

    mp3_file = idx;

    if (xEventGroupGetBits(user_event_group) & AUDIO_PLAYER_RUN_BIT) {
        playback_pending = true;
    } else {
        xEventGroupClearBits(user_event_group, AUDIO_PLAYER_IDLE_BIT);
        xEventGroupSetBits(user_event_group, AUDIO_PLAYER_RUN_BIT);
    }
}

void audio_player_init(void)
{
    if (!(xEventGroupGetBits(user_event_group) & AUDIO_PLAYER_RUN_BIT)) {
        xEventGroupSetBits(user_event_group, AUDIO_PLAYER_IDLE_BIT);
    }

    xTaskCreatePinnedToCore(audio_player_task, "audioPlayerT", 8448, NULL, configMAX_PRIORITIES - 2, NULL, 0);
}
