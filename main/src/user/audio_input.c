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

static void audio_input_task_handle(void *pvParameters)
{
    char *i2s_read_buff = (char *)calloc(100, sizeof(char));
    size_t bytes_read;
    vTaskDelay(2000);
    while (1) {
        i2s_read(I2S_NUM_0, i2s_read_buff, 100, &bytes_read, portMAX_DELAY);

        // Copy data to FIFO
        uint32_t idx = 0;
        const uint8_t *data = (const uint8_t *)i2s_read_buff;
        while (bytes_read > 0) {
            int16_t data_l = data[idx+3] << 8 | data[idx+2];
            int16_t data_r = data[idx+1] << 8 | data[idx];
            vfx_fifo_write((data_l + data_r) / 2);
            idx  += 4;
            bytes_read -= 4;
        }
        vTaskDelay(10);
    }
}

void audio_input_init(void)
{
    xTaskCreate(audio_input_task_handle, "audioInputT", 2048, NULL, 5, NULL);
}
