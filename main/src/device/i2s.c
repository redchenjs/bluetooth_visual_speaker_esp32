/*
 * i2s.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

void i2s0_init(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    i2s_config_t i2s_config = {
#if defined(CONFIG_I2S_OUTPUT_INTERNAL_DAC)
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .use_apll = 0,
#elif defined(CONFIG_I2S_OUTPUT_PDM)
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .use_apll = 0,
#else
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .use_apll = chip_info.revision,                                         // Don't use apll on rev0 chips
#endif
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                // Interrupt level 1
    };
    i2s_driver_install(0, &i2s_config, 0, NULL);
#if defined(CONFIG_I2S_OUTPUT_INTERNAL_DAC) || defined(CONFIG_I2S_OUTPUT_PDM)
    i2s_set_pin(0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
#else
    i2s_pin_config_t pin_config = {
        .bck_io_num = 22,
        .ws_io_num = 21,
        .data_out_num = 19,
        .data_in_num = -1                                                       // Not used
    };
    i2s_set_pin(0, &pin_config);
#endif
}
