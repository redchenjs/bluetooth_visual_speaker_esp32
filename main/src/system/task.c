/*
 * task.c
 *
 *  Created on: 2018-02-16 18:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks/gui_task.h"
#include "tasks/mp3_player.h"
#include "tasks/bt_speaker.h"
#include "tasks/led_indicator.h"

void task_init(void)
{
    xTaskCreate(gui_task, "gui_task", 2048, NULL, 5, NULL);
    xTaskCreate(mp3_player_task, "mp3_player_task", 8192, NULL, 5, NULL);
    xTaskCreate(bt_speaker_task, "bt_speaker_task", 2560, NULL, 5, NULL);
#if !defined(CONFIG_I2S_OUTPUT_INTERNAL_DAC) && !defined(CONFIG_I2S_OUTPUT_PDM)
    xTaskCreate(led_indicator_task, "led_indicator_task", 1024, NULL, 5, NULL);
#endif
}
