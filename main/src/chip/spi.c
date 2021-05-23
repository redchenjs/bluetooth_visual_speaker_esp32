/*
 * spi.c
 *
 *  Created on: 2018-02-10 16:38
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"

#include "chip/spi.h"

#include "board/st7735.h"
#include "board/st7789.h"
#include "board/cube0414.h"

#if defined(CONFIG_ENABLE_VFX) && !defined(CONFIG_VFX_OUTPUT_WS2812)
spi_device_handle_t spi_host;

void spi_host_init(void)
{
    spi_bus_config_t bus_conf = {
#ifdef CONFIG_CUBE0414_RTL_REV_4
        .miso_io_num = CONFIG_SPI_MISO_PIN,
#else
        .miso_io_num = -1,
#endif
        .mosi_io_num = CONFIG_SPI_MOSI_PIN,
        .sclk_io_num = CONFIG_SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
#ifdef CONFIG_VFX_OUTPUT_ST7735
        .max_transfer_sz = ST7735_SCREEN_WIDTH * ST7735_SCREEN_HEIGHT * 2
#elif defined(CONFIG_VFX_OUTPUT_ST7789)
        .max_transfer_sz = ST7789_SCREEN_WIDTH * ST7789_SCREEN_HEIGHT * 2
#else
        .max_transfer_sz = CUBE0414_X * CUBE0414_Y * CUBE0414_Z * 3
#endif
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_NUM, &bus_conf, 1));

    spi_device_interface_config_t dev_conf = {
        .mode = 0,
        .spics_io_num = CONFIG_SPI_CS_PIN,
#ifdef CONFIG_VFX_OUTPUT_ST7735
        .clock_speed_hz = SPI_MASTER_FREQ_26M,
        .pre_cb = st7735_setpin_dc,
#elif defined(CONFIG_VFX_OUTPUT_ST7789)
        .clock_speed_hz = SPI_MASTER_FREQ_40M,
        .pre_cb = st7789_setpin_dc,
#else
        .clock_speed_hz = SPI_MASTER_FREQ_20M,
        .pre_cb = cube0414_setpin_dc,
#endif
        .queue_size = 2,
        .flags = SPI_DEVICE_NO_DUMMY
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_NUM, &dev_conf, &spi_host));

    ESP_LOGI(SPI_HOST_TAG, "initialized, sclk: %d, mosi: %d, miso: %d, cs: %d",
             bus_conf.sclk_io_num,
             bus_conf.mosi_io_num,
             bus_conf.miso_io_num,
             dev_conf.spics_io_num
    );
}
#endif
