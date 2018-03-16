/*
 * init.c
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "device/bt.h"
#include "device/spi.h"
#include "device/i2s.h"
#include "device/gpio.h"
#include "device/spiffs.h"

#include "driver/led.h"

void device_init(void)
{
    bt0_init();
#if !defined(CONFIG_SCREEN_PANEL_NONE)
    spi1_init();
#endif
    i2s0_init();
#if !defined(CONFIG_I2S_OUTPUT_INTERNAL_DAC) && !defined(CONFIG_I2S_OUTPUT_PDM)
    gpio0_init();
#endif
    spiffs0_init();
}

void driver_init(void)
{
#if !defined(CONFIG_I2S_OUTPUT_INTERNAL_DAC)
    led_init();
#endif
}

