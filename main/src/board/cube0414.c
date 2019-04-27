/*
 * cube0414.c
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "driver/spi_master.h"

#include "chip/spi.h"
#include "board/cube0414.h"

#ifdef CONFIG_VFX_OUTPUT_CUBE0414

#define CUBE0414_GPIO_PIN_DC CONFIG_LIGHT_CUBE_DC_PIN

spi_transaction_t spi1_trans[2];

void cube0414_init_board(void)
{
    gpio_set_direction(CUBE0414_GPIO_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(CUBE0414_GPIO_PIN_DC, 0);

    memset(spi1_trans, 0, sizeof(spi1_trans));
}

void cube0414_setpin_dc(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(CUBE0414_GPIO_PIN_DC, dc);
}

void cube0414_write_cmd(uint8_t cmd)
{
    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &cmd;
    spi1_trans[0].user = (void*)0;

    spi_device_transmit(spi1, &spi1_trans[0]);
}

void cube0414_write_data(uint8_t data)
{
    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &data;
    spi1_trans[0].user = (void*)1;

    spi_device_transmit(spi1, &spi1_trans[0]);
}

void cube0414_refresh_gram(uint8_t *gram)
{
    spi1_trans[0].length = 8,
    spi1_trans[0].tx_data[0] = 0xda;    // Write Frame Data
    spi1_trans[0].user = (void*)0;
    spi1_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[1].length = 8*8*8*24;   // 8x8x8x24bit
    spi1_trans[1].tx_buffer = gram;
    spi1_trans[1].user = (void*)1;

    // Queue all transactions.
    for (int x=0; x<2; x++) {
        spi_device_queue_trans(spi1, &spi1_trans[x], portMAX_DELAY);
    }
}
#endif
