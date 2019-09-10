/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "driver/spi_master.h"

#include "board/st7735.h"
#include "board/st7789.h"
#include "board/cube0414.h"

#define HSPI_TAG "hspi"

#ifdef CONFIG_ENABLE_VFX
spi_device_handle_t hspi;

void hspi_init(void)
{
    spi_bus_config_t buscfg={
        .miso_io_num = -1,
        .mosi_io_num = CONFIG_SPI_MOSI_PIN,
        .sclk_io_num = CONFIG_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
#ifdef CONFIG_VFX_OUTPUT_CUBE0414
        .max_transfer_sz = CUBE0414_X*CUBE0414_Y*CUBE0414_Z*3
#elif defined(CONFIG_VFX_OUTPUT_ST7735)
        .max_transfer_sz = ST7735_SCREEN_WIDTH*ST7735_SCREEN_HEIGHT*2
#elif defined(CONFIG_VFX_OUTPUT_ST7789)
        .max_transfer_sz = ST7789_SCREEN_WIDTH*ST7789_SCREEN_HEIGHT*2
#endif
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, 1));

    spi_device_interface_config_t devcfg={
        .mode = 0,                                // SPI mode 0
        .spics_io_num = CONFIG_SPI_CS_PIN,        // CS pin
#ifdef CONFIG_VFX_OUTPUT_CUBE0414
        .clock_speed_hz = 40000000,               // Clock out at 40 MHz
        .queue_size = 2,                          // We want to be able to queue 2 transactions at a time
        .pre_cb = cube0414_setpin_dc,             // Specify pre-transfer callback to handle D/C line
#elif defined(CONFIG_VFX_OUTPUT_ST7735)
        .clock_speed_hz = 26000000,               // Clock out at 26 MHz
        .queue_size = 6,                          // We want to be able to queue 6 transactions at a time
        .pre_cb = st7735_setpin_dc,               // Specify pre-transfer callback to handle D/C line
#elif defined(CONFIG_VFX_OUTPUT_ST7789)
        .clock_speed_hz = 40000000,               // Clock out at 40 MHz
        .queue_size = 6,                          // We want to be able to queue 6 transactions at a time
        .pre_cb = st7789_setpin_dc,               // Specify pre-transfer callback to handle D/C line
#endif
        .flags = SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &hspi));

    ESP_LOGI(HSPI_TAG, "initialized, sclk: %d, mosi: %d, miso: %d, cs: %d",
             buscfg.sclk_io_num,
             buscfg.mosi_io_num,
             buscfg.miso_io_num,
             devcfg.spics_io_num
    );
}
#endif
