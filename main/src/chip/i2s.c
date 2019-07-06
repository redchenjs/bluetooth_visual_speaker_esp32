/*
 * i2s.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "soc/rtc.h"

#define TAG "i2s"

static esp_chip_info_t chip_info;

static int i2s0_sample_rate = 44100;
static int i2s0_bits_per_sample = 16;

void i2s0_init(void)
{
    esp_chip_info(&chip_info);
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX
#ifdef CONFIG_AUDIO_OUTPUT_INTERNAL_DAC
                | I2S_MODE_DAC_BUILT_IN
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
                | I2S_MODE_RX
#endif
#ifdef CONFIG_AUDIO_INPUT_INTERNAL_ADC
                | I2S_MODE_ADC_BUILT_IN
#endif
#if defined(CONFIG_AUDIO_OUTPUT_PDM) || defined(CONFIG_AUDIO_INPUT_PDM)
                | I2S_MODE_PDM
#endif
        ,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB
#if defined(CONFIG_AUDIO_OUTPUT_I2S) && !defined(CONFIG_AUDIO_INPUT_I2S)
                                | I2S_COMM_FORMAT_I2S
#endif
        ,
        .use_apll = chip_info.revision,                                         // Don't use apll on rev0 chips
        .sample_rate = i2s0_sample_rate,
        .bits_per_sample = i2s0_bits_per_sample,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .tx_desc_auto_clear = true                                              // Auto clear tx descriptor on underflow
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
#if defined(CONFIG_AUDIO_OUTPUT_INTERNAL_DAC) || defined(CONFIG_AUDIO_INPUT_INTERNAL_ADC)

#ifdef CONFIG_INTERNAL_DAC_MODE_LEFT
    ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN));
#elif defined(CONFIG_INTERNAL_DAC_MODE_RIGHT)
    ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN));
#else
    ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN));
#endif

#ifdef CONFIG_AUDIO_INPUT_INTERNAL_ADC
    ESP_ERROR_CHECK(i2s_set_adc_mode(CONFIG_INTERNAL_ADC_UNIT, CONFIG_INTERNAL_ADC_CHANNEL));
#endif

#else // defined(CONFIG_AUDIO_OUTPUT_INTERNAL_DAC) || defined(CONFIG_AUDIO_INPUT_INTERNAL_ADC)
    i2s_pin_config_t pin_config = {
#if defined(CONFIG_AUDIO_OUTPUT_I2S) || defined(CONFIG_AUDIO_INPUT_I2S)
        .bck_io_num   = CONFIG_I2S_BCLK_PIN,
        .ws_io_num    = CONFIG_I2S_LRCK_PIN,

#ifdef CONFIG_AUDIO_OUTPUT_I2S
        .data_out_num = CONFIG_I2S_DOUT_PIN,
#else
        .data_out_num = -1,
#endif

#ifdef CONFIG_AUDIO_INPUT_I2S
        .data_in_num  = CONFIG_I2S_DIN_PIN
#else
        .data_in_num  = -1
#endif

#else // defined(CONFIG_AUDIO_OUTPUT_I2S) || defined(CONFIG_AUDIO_INPUT_I2S)
        .bck_io_num   = -1,
        .ws_io_num    = CONFIG_PDM_CLK_PIN,

#ifdef CONFIG_AUDIO_OUTPUT_PDM
        .data_out_num = CONFIG_PDM_DOUT_PIN,
#else
        .data_out_num = -1.
#endif

#ifdef CONFIG_AUDIO_INPUT_PDM
        .data_in_num  = CONFIG_PDM_DIN_PIN
#else
        .data_in_num  = -1
#endif

#endif // defined(CONFIG_AUDIO_OUTPUT_I2S) || defined(CONFIG_AUDIO_INPUT_I2S)
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));
#endif // defined(CONFIG_AUDIO_OUTPUT_INTERNAL_DAC) || defined(CONFIG_AUDIO_INPUT_INTERNAL_ADC)

    ESP_LOGI(TAG, "i2s-0 initialized.");
}

void i2s0_set_sample_rate(int rate)
{
    if (rate != i2s0_sample_rate) {
        i2s_set_sample_rates(I2S_NUM_0, rate);
        i2s0_sample_rate = rate;
    }
}
