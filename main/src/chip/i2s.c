/*
 * i2s.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "driver/i2s.h"

#define I2S0_TAG "i2s-0"
#define I2S1_TAG "i2s-1"

static i2s_config_t i2s_output_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX
#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == CONFIG_AUDIO_INPUT_I2S_NUM)
          | I2S_MODE_RX
#endif
    ,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .use_apll = 1,                                                          // use APLL
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .tx_desc_auto_clear = true,                                             // auto clear tx descriptor on underflow
    .dma_buf_count = 2,
    .dma_buf_len = 128,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT                            // 2-channels
};

#if !defined(CONFIG_AUDIO_INPUT_NONE) && (CONFIG_AUDIO_OUTPUT_I2S_NUM != CONFIG_AUDIO_INPUT_I2S_NUM)
static i2s_config_t i2s_input_config = {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX
#ifdef CONFIG_AUDIO_INPUT_PDM
          | I2S_MODE_PDM
#endif
    ,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .use_apll = 0,                                                          // use PLL_D2
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT                            // 2-channels
};
#endif

#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 0) || (CONFIG_AUDIO_INPUT_I2S_NUM == 0)
static void i2s0_init(void)
{
#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 0)
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_output_config, 0, NULL));
#else
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_input_config, 0, NULL));
#endif

    i2s_pin_config_t pin_config = {
#ifndef CONFIG_AUDIO_INPUT_PDM
        .bck_io_num = CONFIG_I2S0_BCLK_PIN,
        .ws_io_num = CONFIG_I2S0_LRCK_PIN,
#ifdef CONFIG_AUDIO_OUTPUT_I2S0
        .data_out_num = CONFIG_I2S0_DOUT_PIN,
#else
        .data_out_num = -1,
#endif
#ifdef CONFIG_AUDIO_INPUT_I2S0
        .data_in_num = CONFIG_I2S0_DIN_PIN
#else
        .data_in_num = -1
#endif
#else // #ifndef CONFIG_AUDIO_INPUT_PDM
        .bck_io_num = -1,
        .ws_io_num = CONFIG_PDM_CLK_PIN,
        .data_out_num = -1,
        .data_in_num = CONFIG_PDM_DIN_PIN
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

static void i2s0_deinit(void)
{
    ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_NUM_0));

    ESP_LOGI(I2S0_TAG, "deinitialized.");
}
#endif

#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 1) || (CONFIG_AUDIO_INPUT_I2S_NUM == 1)
static void i2s1_init(void)
{
#if (CONFIG_AUDIO_OUTPUT_I2S_NUM == 1)
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2s_output_config, 0, NULL));
#else
    ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_1, &i2s_input_config, 0, NULL));
#endif

    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S1_BCLK_PIN,
        .ws_io_num = CONFIG_I2S1_LRCK_PIN,
#ifdef CONFIG_AUDIO_OUTPUT_I2S1
        .data_out_num = CONFIG_I2S1_DOUT_PIN,
#else
        .data_out_num = -1,
#endif
#ifdef CONFIG_AUDIO_INPUT_I2S1
        .data_in_num = CONFIG_I2S1_DIN_PIN
#else
        .data_in_num = -1
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

static void i2s1_deinit(void)
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

void i2s_output_set_sample_rate(unsigned int sample_rate)
{
    if (sample_rate != i2s_output_config.sample_rate) {
        i2s_output_config.sample_rate = sample_rate;
        i2s_zero_dma_buffer(CONFIG_AUDIO_OUTPUT_I2S_NUM);
        i2s_set_sample_rates(CONFIG_AUDIO_OUTPUT_I2S_NUM, i2s_output_config.sample_rate);
    }
}

#ifndef CONFIG_AUDIO_INPUT_NONE
void i2s_input_init(void)
{
#if (CONFIG_AUDIO_INPUT_I2S_NUM == 0)
    i2s0_init();
#else
    i2s1_init();
#endif
}

void i2s_input_deinit(void)
{
#if (CONFIG_AUDIO_INPUT_I2S_NUM == 0)
    i2s0_deinit();
#else
    i2s1_deinit();
#endif
}
#endif
