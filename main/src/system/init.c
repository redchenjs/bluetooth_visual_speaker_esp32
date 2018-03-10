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
    spi1_init();
    i2s0_init();
    gpio0_init();
    spiffs0_init();
}

void driver_init(void)
{
    led_init();
}

