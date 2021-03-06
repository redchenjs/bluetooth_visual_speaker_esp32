/*
 * ws2812.c
 *
 *  Created on: 2020-06-25 22:01
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "led_strip.h"
#include "driver/rmt.h"

#include "board/ws2812.h"

#ifdef CONFIG_VFX_OUTPUT_WS2812

#define TAG "ws2812"

static led_strip_t *strip = NULL;

void ws2812_init_board(void)
{
    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_CHANNEL_0,
        .gpio_num = CONFIG_WS2812_DIN_PIN,
        .clk_div = 2,
        .mem_block_num = 1,
        .flags = 0
    };
    rmt_config(&config);

    rmt_driver_install(RMT_CHANNEL_0, 0, 0);

    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(WS2812_X * WS2812_Y * WS2812_Z, RMT_CHANNEL_0);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "initialization failed.");
    } else {
        ESP_LOGI(TAG, "initialized, cas: %d, din: %d", WS2812_Z, CONFIG_WS2812_DIN_PIN);
    }
}

void ws2812_set_pixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    strip->set_pixel(strip, index, red, green, blue);
}

void ws2812_refresh(void)
{
    strip->refresh(strip, 8);
}

void ws2812_clear(void)
{
    strip->clear(strip, 8);
}
#endif
