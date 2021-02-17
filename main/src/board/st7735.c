/*
 * st7735.c
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"

#include "drivers/gdisp/ST7735/ST7735.h"

#include "chip/spi.h"
#include "board/st7735.h"

#ifdef CONFIG_VFX_OUTPUT_ST7735

#define TAG "st7735"

static spi_transaction_t hspi_trans[2];

void st7735_init_board(void)
{
    memset(hspi_trans, 0x00, sizeof(hspi_trans));

    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(CONFIG_LCD_DC_PIN) | BIT64(CONFIG_LCD_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 40000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = CONFIG_LCD_BL_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint = 0,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);

    ledc_fade_func_install(0);

    ESP_LOGI(TAG, "initialized, bl: %d, dc: %d, rst: %d", CONFIG_LCD_BL_PIN, CONFIG_LCD_DC_PIN, CONFIG_LCD_RST_PIN);
}

void st7735_set_backlight(uint8_t val)
{
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, val, 500, LEDC_FADE_NO_WAIT);
}

void st7735_setpin_dc(spi_transaction_t *t)
{
    gpio_set_level(CONFIG_LCD_DC_PIN, (int)t->user);
}

void st7735_setpin_reset(uint8_t val)
{
    gpio_set_level(CONFIG_LCD_RST_PIN, val);
}

void st7735_write_cmd(uint8_t cmd)
{
    hspi_trans[0].length = 8;
    hspi_trans[0].tx_buffer = &cmd;
    hspi_trans[0].user = (void *)0;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void st7735_write_data(uint8_t data)
{
    hspi_trans[0].length = 8;
    hspi_trans[0].tx_buffer = &data;
    hspi_trans[0].user = (void *)1;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void st7735_write_buff(uint8_t *buff, uint32_t n)
{
    hspi_trans[0].length = n * 8;
    hspi_trans[0].tx_buffer = buff;
    hspi_trans[0].user = (void *)1;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void st7735_refresh_gram(uint8_t *gram)
{
    hspi_trans[0].length = 8,
    hspi_trans[0].tx_data[0] = ST7735_RAMWR;
    hspi_trans[0].user = (void *)0;
    hspi_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi_device_queue_trans(hspi, &hspi_trans[0], portMAX_DELAY);

    hspi_trans[1].length = ST7735_SCREEN_WIDTH * ST7735_SCREEN_HEIGHT * 2 * 8;
    hspi_trans[1].tx_buffer = gram;
    hspi_trans[1].user = (void *)1;

    spi_device_queue_trans(hspi, &hspi_trans[1], portMAX_DELAY);
}
#endif
