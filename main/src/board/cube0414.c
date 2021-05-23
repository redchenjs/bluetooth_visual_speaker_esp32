/*
 * cube0414.c
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "driver/gpio.h"
#include "drivers/gdisp/CUBE0414/CUBE0414.h"

#include "chip/spi.h"
#include "board/cube0414.h"

#ifdef CONFIG_VFX_OUTPUT_CUBE0414

#define TAG "cube0414"

static spi_transaction_t spi_trans[2] = {0};

void cube0414_init_board(void)
{
    gpio_config_t io_conf = {
#ifndef CONFIG_CUBE0414_RTL_REV_2
        .pin_bit_mask = BIT64(CONFIG_CUBE0414_DC_PIN) | BIT64(CONFIG_CUBE0414_RST_PIN),
#else
        .pin_bit_mask = BIT64(CONFIG_CUBE0414_DC_PIN),
#endif
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

#ifndef CONFIG_CUBE0414_RTL_REV_2
    ESP_LOGI(TAG, "initialized, dc: %d, rst: %d", CONFIG_CUBE0414_DC_PIN, CONFIG_CUBE0414_RST_PIN);
#else
    ESP_LOGI(TAG, "initialized, dc: %d", CONFIG_CUBE0414_DC_PIN);
#endif
}

void cube0414_setpin_dc(spi_transaction_t *t)
{
    gpio_set_level(CONFIG_CUBE0414_DC_PIN, (int)t->user);
}

#ifndef CONFIG_CUBE0414_RTL_REV_2
void cube0414_setpin_reset(uint8_t val)
{
    gpio_set_level(CONFIG_CUBE0414_RST_PIN, val);
}
#endif

void cube0414_write_cmd(uint8_t cmd)
{
    spi_trans[0].length = 8;
    spi_trans[0].rxlength = 0;
    spi_trans[0].tx_buffer = &cmd;
    spi_trans[0].rx_buffer = NULL;
    spi_trans[0].user = (void *)0;
    spi_trans[0].flags = 0;

    spi_device_transmit(spi_host, &spi_trans[0]);
}

void cube0414_write_data(uint8_t data)
{
    spi_trans[0].length = 8;
    spi_trans[0].rxlength = 0;
    spi_trans[0].tx_buffer = &data;
    spi_trans[0].rx_buffer = NULL;
    spi_trans[0].user = (void *)1;
    spi_trans[0].flags = 0;

    spi_device_transmit(spi_host, &spi_trans[0]);
}

void cube0414_write_buff(uint8_t *buff, uint32_t n)
{
    spi_trans[0].length = n * 8;
    spi_trans[0].rxlength = 0;
    spi_trans[0].tx_buffer = buff;
    spi_trans[0].rx_buffer = NULL;
    spi_trans[0].user = (void *)1;
    spi_trans[0].flags = 0;

    spi_device_transmit(spi_host, &spi_trans[0]);
}

void cube0414_read_buff(uint8_t *buff, uint32_t n)
{
    spi_trans[0].length = n * 8;
    spi_trans[0].rxlength = n * 8;
    spi_trans[0].tx_buffer = NULL;
    spi_trans[0].rx_buffer = buff;
    spi_trans[0].user = (void *)1;
    spi_trans[0].flags = 0;

    spi_device_transmit(spi_host, &spi_trans[0]);
}

void cube0414_refresh_gram(uint8_t *gram)
{
    spi_trans[0].length = 8;
    spi_trans[0].rxlength = 0;
    spi_trans[0].tx_data[0] = CUBE0414_DATA_WR;
    spi_trans[0].rx_buffer = NULL;
    spi_trans[0].user = (void *)0;
    spi_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi_device_queue_trans(spi_host, &spi_trans[0], portMAX_DELAY);

    spi_trans[1].length = CUBE0414_X * CUBE0414_Y * CUBE0414_Z * 3 * 8;
    spi_trans[1].rxlength = 0;
    spi_trans[1].tx_buffer = gram;
    spi_trans[1].rx_buffer = NULL;
    spi_trans[1].user = (void *)1;
    spi_trans[1].flags = 0;

    spi_device_queue_trans(spi_host, &spi_trans[1], portMAX_DELAY);
}
#endif
