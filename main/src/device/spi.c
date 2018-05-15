/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/spi_master.h"
#include "driver/cube0414.h"

spi_device_handle_t spi1;

void spi1_init(void)
{
    esp_err_t ret;

    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=18,
        .sclk_io_num=5,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=CUBE0414_X*CUBE0414_Y*CUBE0414_Z*3
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=40000000,               // Clock out at 40 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=27,                       // CS pin
        .queue_size=2,                          // We want to be able to queue 6 transactions at a time
        .pre_cb=cube0414_setpin_dc,             // Specify pre-transfer callback to handle D/C line
        .flags=SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX
    };
    // Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    // Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi1);
    assert(ret==ESP_OK);
}
