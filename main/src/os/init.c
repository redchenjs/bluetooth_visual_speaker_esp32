/*
 * init.c
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "os/event.h"
#include "chip/bt.h"
#include "chip/nvs.h"
#include "chip/spi.h"
#include "chip/i2s.h"
#include "user/bt.h"
#include "user/led.h"
#include "user/vfx.h"
#include "user/audio.h"

void os_init(void)
{
    user_event_group = xEventGroupCreate();
}

void chip_init(void)
{
    nvs_init();
    bt_init();
    spi1_init();
    i2s0_init();
}

void board_init(void) {}

void user_init(void)
{
    xTaskCreate(bt_task, "bt_task", 2560, NULL, 6, NULL);

#ifdef CONFIG_ENABLE_VFX
    xTaskCreate(vfx_task, "vfx_task", 4096, NULL, 5, NULL);
#endif

#ifdef CONFIG_ENABLE_LED
    xTaskCreate(led_task, "led_task", 1024, NULL, 5, NULL);
#endif

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    xTaskCreate(audio_task, "audio_task", 8448, NULL, 5, NULL);
#endif
}
