/*
 * user.c
 *
 *  Created on: 2018-02-16 18:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user/bt_daemon.h"
#include "user/ble_daemon.h"
#include "user/led_daemon.h"
#include "user/gui_daemon.h"
#include "user/audio_daemon.h"

void user_init(void)
{
    xTaskCreate(bt_daemon, "bt_daemon", 2560, NULL, 6, NULL);
    xTaskCreate(ble_daemon, "ble_daemon", 1024, NULL, 6, NULL);
    xTaskCreate(led_daemon, "led_daemon", 1024, NULL, 5, NULL);
    xTaskCreate(gui_daemon, "gui_daemon", 4096, NULL, 5, NULL);
    xTaskCreate(audio_daemon, "audio_daemon", 8448, NULL, 5, NULL);
}
