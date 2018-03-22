/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "driver/spi_master.h"
#include "driver/ssd1331.h"
#include "driver/ssd1351.h"
#include "driver/st7735.h"

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
#if defined(CONFIG_SCREEN_PANEL_SSD1331)
        .max_transfer_sz=SSD1331_SCREEN_WIDTH*SSD1331_SCREEN_HEIGHT*2
#elif defined(CONFIG_SCREEN_PANEL_SSD1351)
        .max_transfer_sz=SSD1351_SCREEN_WIDTH*SSD1351_SCREEN_HEIGHT*2
#else
        .max_transfer_sz=ST7735_SCREEN_WIDTH*ST7735_SCREEN_HEIGHT*2
#endif
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=20000000,               // Clock out at 20 MHz
        .mode=0,                                // SPI mode 0
        .spics_io_num=27,                       // CS pin
#if defined(CONFIG_SCREEN_PANEL_SSD1331)
        .queue_size=3,                          // We want to be able to queue 3 transactions at a time
        .pre_cb=ssd1331_setpin_dc,              // Specify pre-transfer callback to handle D/C line
#elif defined(CONFIG_SCREEN_PANEL_SSD1351)
        .queue_size=6,                          // We want to be able to queue 6 transactions at a time
        .pre_cb=ssd1351_setpin_dc,              // Specify pre-transfer callback to handle D/C line
#else
        .queue_size=6,                          // We want to be able to queue 6 transactions at a time
        .pre_cb=st7735_setpin_dc,               // Specify pre-transfer callback to handle D/C line
#endif
        .flags=SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX
    };
    // Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    // Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi1);
    assert(ret==ESP_OK);
}
