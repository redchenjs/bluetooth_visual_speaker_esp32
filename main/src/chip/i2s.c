/*
 * i2s.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

#define I2S0_TAG "i2s-0"
#define I2S1_TAG "i2s-1"

static int i2s_output_sample_rate = 44100;
static int i2s_output_bits_per_sample = 16;

static int i2s_input_sample_rate = 44100;
static int i2s_input_bits_per_sample = 16;

#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 0) || (CONFIG_AUDIO_INPUT_I2S_NUM == 0)
void i2s0_init(void)
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER
#ifdef CONFIG_AUDIO_OUTPUT_I2S0
                | I2S_MODE_TX
#endif
#if defined(CONFIG_AUDIO_INPUT_I2S0) || defined(CONFIG_AUDIO_INPUT_PDM)
                | I2S_MODE_RX
#endif
#ifdef CONFIG_AUDIO_INPUT_PDM
                | I2S_MODE_PDM
#endif
        ,
#ifdef CONFIG_AUDIO_INPUT_PDM
        .communication_format = I2S_COMM_FORMAT_PCM,
#else
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
#endif
#ifdef CONFIG_AUDIO_OUTPUT_I2S0
        .use_apll = 1,                                                          // Use APLL
        .sample_rate = i2s_output_sample_rate,
        .bits_per_sample = i2s_output_bits_per_sample,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3,
        .tx_desc_auto_clear = true,                                             // Auto clear tx descriptor on underflow
        .dma_buf_count = 8,
        .dma_buf_len = 128,
#else
        .use_apll = 0,                                                          // Use PLL_D2
        .sample_rate = i2s_input_sample_rate,
        .bits_per_sample = i2s_input_bits_per_sample,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
#endif
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    i2s_pin_config_t pin_config = {
#if defined(CONFIG_AUDIO_OUTPUT_I2S0) || defined(CONFIG_AUDIO_INPUT_I2S0)
        .bck_io_num   = CONFIG_I2S0_BCLK_PIN,
        .ws_io_num    = CONFIG_I2S0_LRCK_PIN,
#ifdef CONFIG_AUDIO_OUTPUT_I2S0
        .data_out_num = CONFIG_I2S0_DOUT_PIN,
#else
        .data_out_num = -1,
#endif
#ifdef CONFIG_AUDIO_INPUT_I2S0
        .data_in_num  = CONFIG_I2S0_DIN_PIN
#else
        .data_in_num  = -1
#endif
#else
        .bck_io_num   = -1,
        .ws_io_num    = CONFIG_PDM_CLK_PIN,
        .data_out_num = -1,
        .data_in_num  = CONFIG_PDM_DIN_PIN
#endif
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_0, &pin_config));

    ESP_LOGI(I2S0_TAG, "initialized, bck: %d, ws: %d, dout: %d, din: %d",
             pin_config.bck_io_num,
             pin_config.ws_io_num,
             pin_config.data_out_num,
             pin_config.data_in_num
    );
}

void i2s0_deinit(void)
{
    ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_NUM_0));

    ESP_LOGI(I2S0_TAG, "deinitialized.");
}
#endif

#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 1) || (CONFIG_AUDIO_INPUT_I2S_NUM == 1)
void i2s1_init(void)
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER
#ifdef CONFIG_AUDIO_OUTPUT_I2S1
                | I2S_MODE_TX
#endif
#ifdef CONFIG_AUDIO_INPUT_I2S1
                | I2S_MODE_RX
#endif
        ,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB | I2S_COMM_FORMAT_I2S,
#ifdef CONFIG_AUDIO_OUTPUT_I2S1
        .use_apll = 1,                                                          // Use APLL
        .sample_rate = i2s_output_sample_rate,
        .bits_per_sample = i2s_output_bits_per_sample,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3,
        .tx_desc_auto_clear = true,                                             // Auto clear tx descriptor on underflow
        .dma_buf_count = 8,
        .dma_buf_len = 128,
#else
        .use_apll = 0,                                                          // Use PLL_D2
        .sample_rate = i2s_input_sample_rate,
        .bits_per_sample = i2s_input_bits_per_sample,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
#endif
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           // 2-channels
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL));
    i2s_pin_config_t pin_config = {
        .bck_io_num   = CONFIG_I2S1_BCLK_PIN,
        .ws_io_num    = CONFIG_I2S1_LRCK_PIN,
#ifdef CONFIG_AUDIO_OUTPUT_I2S1
        .data_out_num = CONFIG_I2S1_DOUT_PIN,
#else
        .data_out_num = -1,
#endif
#ifdef CONFIG_AUDIO_INPUT_I2S1
        .data_in_num  = CONFIG_I2S1_DIN_PIN
#else
        .data_in_num  = -1
#endif
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_1, &pin_config));

    ESP_LOGI(I2S1_TAG, "initialized, bck: %d, ws: %d, dout: %d, din: %d",
             pin_config.bck_io_num,
             pin_config.ws_io_num,
             pin_config.data_out_num,
             pin_config.data_in_num
    );
}

void i2s1_deinit(void)
{
    ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_NUM_1));

    ESP_LOGI(I2S1_TAG, "deinitialized.");
}
#endif

void i2s_output_init(void)
{
#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 0)
    i2s0_init();
#else
    i2s1_init();
#endif
}

void i2s_output_deinit(void)
{
#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 0)
    i2s0_deinit();
#else
    i2s1_deinit();
#endif
}

void i2s_set_output_sample_rate(int rate)
{
    if (rate != i2s_output_sample_rate) {
        i2s_set_sample_rates(CONFIG_AUDIO_OUTPUT_I2S_NUM, rate);
        i2s_output_sample_rate = rate;
    }
}

void i2s_set_input_sample_rate(int rate)
{
#if !defined(CONFIG_AUDIO_INPUT_NONE) && (CONFIG_AUDIO_OUTPUT_I2S_NUM != CONFIG_AUDIO_INPUT_I2S_NUM)
    if (rate != i2s_input_sample_rate) {
        i2s_set_sample_rates(CONFIG_AUDIO_INPUT_I2S_NUM, rate);
        i2s_input_sample_rate = rate;
    }
#endif
}

int i2s_get_output_sample_rate(void)
{
    return i2s_output_sample_rate;
}

int i2s_get_input_sample_rate(void)
{
    return i2s_input_sample_rate;
}

int i2s_get_output_bits_per_sample(void)
{
    return i2s_output_bits_per_sample;
}

int i2s_get_input_bits_per_sample(void)
{
    return i2s_input_bits_per_sample;
}
