/*
 * ws2812.c
 *
 *  Created on: 2020-06-25 22:01
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "driver/rmt.h"
#include "board/ws2812.h"

#ifdef CONFIG_VFX_OUTPUT_WS2812

#define TAG "ws2812"

static rmt_item32_t bit0 = {{{ 0, 1, 0, 0 }}};
static rmt_item32_t bit1 = {{{ 0, 1, 0, 0 }}};

static void ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    while (*translated_size < src_size && *item_num < wanted_num) {
        uint8_t temp = ((uint8_t *)src)[(*translated_size)++];

        for (int i = 0; i < 8; i++) {
            if (temp & 0x80) {
                (dest++)->val = bit1.val;
            } else {
                (dest++)->val = bit0.val;
            }

            temp <<= 1;
        }

        *item_num += 8;
    }
}

void ws2812_init_board(void)
{
    uint32_t clock_hz = 0;

    rmt_config_t rmt_conf = RMT_DEFAULT_CONFIG_TX(CONFIG_WS2812_DIN0_PIN, RMT_CHANNEL_0);
    rmt_conf.mem_block_num = 3;
    rmt_conf.clk_div = 2;

    rmt_config(&rmt_conf);
    rmt_driver_install(RMT_CHANNEL_0, 0, 0);

    rmt_get_counter_clock(RMT_CHANNEL_0, &clock_hz);
    bit0.duration0 = clock_hz * CONFIG_LED_T0H_TIME / 1e8;
    bit0.duration1 = clock_hz * CONFIG_LED_T0L_TIME / 1e8;
    bit1.duration0 = clock_hz * CONFIG_LED_T1H_TIME / 1e8;
    bit1.duration1 = clock_hz * CONFIG_LED_T1L_TIME / 1e8;

    rmt_translator_init(RMT_CHANNEL_0, ws2812_rmt_adapter);

#ifndef CONFIG_WS2812_DIN1_PIN
    ESP_LOGI(TAG, "initialized, din0: %d", CONFIG_WS2812_DIN0_PIN);
#else
    rmt_conf.channel = RMT_CHANNEL_4;
    rmt_conf.gpio_num = CONFIG_WS2812_DIN1_PIN;

    rmt_config(&rmt_conf);
    rmt_driver_install(RMT_CHANNEL_4, 0, 0);

    rmt_translator_init(RMT_CHANNEL_4, ws2812_rmt_adapter);

    ESP_LOGI(TAG, "initialized, din0: %d, din1: %d", CONFIG_WS2812_DIN0_PIN, CONFIG_WS2812_DIN1_PIN);
#endif
}

void ws2812_refresh_gram(uint8_t *gram)
{
#ifndef CONFIG_WS2812_DIN1_PIN
    rmt_write_sample(RMT_CHANNEL_0, gram, WS2812_X * WS2812_Y * WS2812_Z * 3, true);
#else
    rmt_write_sample(RMT_CHANNEL_0, gram, WS2812_X * WS2812_Y * 4 * 3, false);
    rmt_write_sample(RMT_CHANNEL_4, gram + WS2812_X * WS2812_Y * 4 * 3, WS2812_X * WS2812_Y * (WS2812_Z - 4) * 3, true);

    rmt_wait_tx_done(RMT_CHANNEL_0, portMAX_DELAY);
#endif
}
#endif
