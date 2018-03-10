/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/spi_master.h"

spi_device_handle_t spi1;
spi_transaction_t spi1_t;

extern void (*spi1_pre_transfer_callback)(spi_transaction_t *);

void spi1_init(void)
{
    esp_err_t ret;

    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=18,
        .sclk_io_num=5,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=96*64*2
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=26000000,               //Clock out at 26 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=27,                       //CS pin
        .queue_size=3,                          //We want to be able to queue 3 transactions at a time
        .pre_cb=spi1_pre_transfer_callback,     //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi1);
    assert(ret==ESP_OK);
}
