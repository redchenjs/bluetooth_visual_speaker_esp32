/*
 * init.c
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "os/event.h"

#include "chip/bt.h"
#include "chip/nvs.h"
#include "chip/spi.h"
#include "chip/i2s.h"

#include "user/led.h"
#include "user/vfx.h"
#include "user/fifo.h"
#include "user/audio.h"
#include "user/bt_app.h"
#include "user/ble_gatts.h"

void os_init(void)
{
    event_init();
}

void chip_init(void)
{
    nvs_init();

    bt_init();

    i2s0_init();

#ifdef CONFIG_ENABLE_VFX
    spi1_init();
#endif
}

void board_init(void) {}

void user_init(void)
{
    bt_app_init();

#ifdef CONFIG_ENABLE_VFX
    ble_gatts_init();

    fifo_init();

    vfx_init();
#endif

#ifdef CONFIG_ENABLE_LED
    led_init();
#endif

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    audio_init();
#endif
}
