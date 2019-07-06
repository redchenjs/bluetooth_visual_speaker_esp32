/*
 * audio_input.c
 *
 *  Created on: 2019-07-05 21:22
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#include "os/core.h"
#include "chip/i2s.h"
#include "user/vfx_core.h"
#include "user/audio_render.h"

#define BUFF_SIZE 128

static uint8_t audio_input_mode = 0;

static void audio_input_task_handle(void *pvParameters)
{
#ifndef CONFIG_AUDIO_INPUT_NONE
    char *read_buff = (char *)malloc(BUFF_SIZE * 8 * sizeof(char));
    size_t read_bytes;

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            AUDIO_INPUT_RUN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        i2s_read(CONFIG_AUDIO_INPUT_I2S_NUM, read_buff, BUFF_SIZE, &read_bytes, portMAX_DELAY);

        // Copy data to FIFO
        uint32_t idx = 0;
        int32_t size = read_bytes;
        const uint8_t *data = (const uint8_t *)read_buff;
        while (size > 0) {
            int16_t data_l = data[idx+3] << 8 | data[idx+2];
            int16_t data_r = data[idx+7] << 8 | data[idx+6];
            vfx_fifo_write((data_l + data_r) / 2);
            idx  += 8;
            size -= 8;
        }
    }
#endif
}

void audio_input_set_mode(uint8_t mode)
{
    audio_input_mode = mode;
    if (audio_input_mode == 0) {
        xEventGroupClearBits(user_event_group, AUDIO_INPUT_RUN_BIT);
    } else {
        xEventGroupSetBits(user_event_group, AUDIO_INPUT_RUN_BIT);
    }
}

uint8_t audio_input_get_mode(void)
{
    return audio_input_mode;
}

void audio_input_init(void)
{
    xTaskCreate(audio_input_task_handle, "audioInputT", 2048, NULL, 5, NULL);
}
