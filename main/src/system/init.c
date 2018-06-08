/*
 * init.c
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "device/bt.h"
#include "device/nvs.h"
#include "device/spi.h"
#include "device/i2s.h"
#include "device/fifo.h"

#include "driver/led.h"

void device_init(void)
{
    nvs_init();
    bt_init();
    spi1_init();
    i2s0_init();
    fifo_init();
}

void driver_init(void)
{
#if defined(CONFIG_ENABLE_LED)
    led_init();
#endif
}
