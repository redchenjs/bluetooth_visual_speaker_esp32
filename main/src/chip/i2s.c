/*
 * i2s.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#define TAG "i2s"

static int i2s0_sample_rate = 44100;
static int i2s0_bits_per_sample = 16;

void i2s0_init(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                  // Only TX
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .use_apll = chip_info.revision,                                         // Don't use apll on rev0 chips
        .sample_rate = i2s0_sample_rate,
        .bits_per_sample = i2s0_bits_per_sample,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
        .dma_buf_count = 8,
        .dma_buf_len = 64
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num   = CONFIG_I2S_BCLK_PIN,
        .ws_io_num    = CONFIG_I2S_LRCK_PIN,
        .data_out_num = CONFIG_I2S_DOUT_PIN,
        .data_in_num  = -1                                                     // Not used
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
    ESP_LOGI(TAG, "i2s-0 initialized.");
}

void i2s0_set_sample_rate(int rate)
{
    if (rate != i2s0_sample_rate) {
        i2s_set_sample_rates(I2S_NUM_0, rate);
        i2s0_sample_rate = rate;
    }
}
