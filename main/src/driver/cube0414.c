/*
 * cube0414.c
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "device/spi.h"
#include "driver/cube0414.h"

#define CUBE0414_GPIO_PIN_DC 23

spi_transaction_t spi1_trans[6];

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
    esp_err_t ret;

    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &cmd;
    spi1_trans[0].user = (void*)0;

    ret = spi_device_transmit(spi1, &spi1_trans[0]);
    assert(ret == ESP_OK);
}

void cube0414_write_data(uint8_t data)
{
    esp_err_t ret;

    spi1_trans[0].length = 8;
    spi1_trans[0].tx_buffer = &data;
    spi1_trans[0].user = (void*)1;

    ret = spi_device_transmit(spi1, &spi1_trans[0]);
    assert(ret == ESP_OK);
}

void cube0414_refresh_gram(uint8_t *gram)
{
    esp_err_t ret;

    spi1_trans[0].length = 8,
    spi1_trans[0].tx_data[0] = 0xda;    // Write Frame Data
    spi1_trans[0].user = (void*)0;
    spi1_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi1_trans[1].length = 8*8*8*24;   // 8x8x8x24bit
    spi1_trans[1].tx_buffer = gram;
    spi1_trans[1].user = (void*)1;

    //Queue all transactions.
    for (int x=0; x<2; x++) {
        ret=spi_device_queue_trans(spi1, &spi1_trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }

    for (int x=0; x<2; x++) {
        spi_transaction_t* ptr;
        ret=spi_device_get_trans_result(spi1, &ptr, portMAX_DELAY);
        assert(ret==ESP_OK);
        assert(ptr==spi1_trans+x);
    }
}
