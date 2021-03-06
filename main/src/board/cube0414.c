/*
 * cube0414.c
 *
 *  Created on: 2018-03-16 16:15
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "drivers/gdisp/CUBE0414/CUBE0414.h"

#include "chip/spi.h"
#include "board/cube0414.h"

#ifdef CONFIG_VFX_OUTPUT_CUBE0414

#define TAG "cube0414"

static spi_transaction_t hspi_trans[2];

void cube0414_init_board(void)
{
    memset(hspi_trans, 0x00, sizeof(hspi_trans));

    gpio_config_t io_conf = {
#ifdef CONFIG_CUBE0414_RTL_REV_3
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

#ifdef CONFIG_CUBE0414_RTL_REV_3
    ESP_LOGI(TAG, "initialized, dc: %d, rst: %d", CONFIG_CUBE0414_DC_PIN, CONFIG_CUBE0414_RST_PIN);
#else
    ESP_LOGI(TAG, "initialized, dc: %d", CONFIG_CUBE0414_DC_PIN);
#endif
}

void cube0414_setpin_dc(spi_transaction_t *t)
{
    gpio_set_level(CONFIG_CUBE0414_DC_PIN, (int)t->user);
}

#ifdef CONFIG_CUBE0414_RTL_REV_3
void cube0414_setpin_reset(uint8_t val)
{
    gpio_set_level(CONFIG_CUBE0414_RST_PIN, val);
}
#endif

void cube0414_write_cmd(uint8_t cmd)
{
    hspi_trans[0].length = 8;
    hspi_trans[0].tx_buffer = &cmd;
    hspi_trans[0].user = (void *)0;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void cube0414_write_data(uint8_t data)
{
    hspi_trans[0].length = 8;
    hspi_trans[0].tx_buffer = &data;
    hspi_trans[0].user = (void *)1;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void cube0414_write_buff(uint8_t *buff, uint32_t n)
{
    hspi_trans[0].length = n * 8;
    hspi_trans[0].tx_buffer = buff;
    hspi_trans[0].user = (void *)1;

    spi_device_transmit(hspi, &hspi_trans[0]);
}

void cube0414_refresh_gram(uint8_t *gram)
{
    hspi_trans[0].length = 8,
    hspi_trans[0].tx_data[0] = CUBE0414_DATA_WR;
    hspi_trans[0].user = (void *)0;
    hspi_trans[0].flags = SPI_TRANS_USE_TXDATA;

    spi_device_queue_trans(hspi, &hspi_trans[0], portMAX_DELAY);

    hspi_trans[1].length = CUBE0414_X * CUBE0414_Y * CUBE0414_Z * 3 * 8;
    hspi_trans[1].tx_buffer = gram;
    hspi_trans[1].user = (void *)1;

    spi_device_queue_trans(hspi, &hspi_trans[1], portMAX_DELAY);
}
#endif
