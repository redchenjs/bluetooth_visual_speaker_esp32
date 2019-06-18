/*
 * st7789.c
 *
 *  Created on: 2018-03-16 12:30
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "chip/spi.h"
#include "board/st7789.h"

#ifdef CONFIG_VFX_OUTPUT_ST7789

#define ST7789_GPIO_PIN_DC  CONFIG_SCREEN_PANEL_DC_PIN
#define ST7789_GPIO_PIN_RST CONFIG_SCREEN_PANEL_RST_PIN

spi_transaction_t spi1_trans[6];

void st7789_init_board(void)
{
    gpio_set_direction(ST7789_GPIO_PIN_DC,  GPIO_MODE_OUTPUT);
    gpio_set_direction(ST7789_GPIO_PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(ST7789_GPIO_PIN_DC,  0);
    gpio_set_level(ST7789_GPIO_PIN_RST, 0);

    memset(spi1_trans, 0, sizeof(spi1_trans));
}

void st7789_setpin_dc(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(ST7789_GPIO_PIN_DC, dc);
}

void st7789_setpin_reset(uint8_t rst)
{
    gpio_set_level(ST7789_GPIO_PIN_RST, rst);
}

void st7789_write_cmd(uint8_t cmd)
{
    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &cmd;
    spi1_trans[0].user = (void*)0;

    spi_device_transmit(spi1, &spi1_trans[0]);
}

void st7789_write_data(uint8_t data)
{
    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &data;
    spi1_trans[0].user = (void*)1;

    spi_device_transmit(spi1, &spi1_trans[0]);
}

void st7789_refresh_gram(uint8_t *gram)
{
    spi1_trans[0].length = 8;
    spi1_trans[0].tx_data[0] = 0x2a;    // Set Column Address
    spi1_trans[0].user = (void*)0;
    spi1_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[1].length = 4*8;
    spi1_trans[1].tx_data[0] = 0x00;    // startx high byte
    spi1_trans[1].tx_data[1] = 0x28;    // startx low byte
    spi1_trans[1].tx_data[2] = 0x01;    // endx high byte
    spi1_trans[1].tx_data[3] = 0x17;    // endx low byte
    spi1_trans[1].user = (void*)1;
    spi1_trans[1].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[2].length = 8,
    spi1_trans[2].tx_data[0] = 0x2b;    // Set Row Address
    spi1_trans[2].user = (void*)0;
    spi1_trans[2].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[3].length = 4*8,
    spi1_trans[3].tx_data[0] = 0x00;    // starty high byte
    spi1_trans[3].tx_data[1] = 0x35;    // starty low byte
    spi1_trans[3].tx_data[2] = 0x00;    // endy high byte
    spi1_trans[3].tx_data[3] = 0xBB;    // endy low byte
    spi1_trans[3].user = (void*)1;
    spi1_trans[3].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[4].length = 8,
    spi1_trans[4].tx_data[0] = 0x2c;    // Set Write RAM
    spi1_trans[4].user = (void*)0;
    spi1_trans[4].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[5].length = ST7789_SCREEN_WIDTH*ST7789_SCREEN_HEIGHT*2*8;
    spi1_trans[5].tx_buffer = gram;
    spi1_trans[5].user = (void*)1;

    // Queue all transactions.
    for (int x=0; x<6; x++) {
        spi_device_queue_trans(spi1, &spi1_trans[x], portMAX_DELAY);
    }
}
#endif
