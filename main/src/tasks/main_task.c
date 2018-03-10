/*
 * main_task.c
 *
 *  Created on: 2018-02-16 18:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "tasks/bt_speaker.h"
#include "tasks/mp3_player.h"
#include "tasks/oled_display.h"
#include "tasks/led_indicator.h"

EventGroupHandle_t task_event_group;

void main_task(void)
{
    task_event_group = xEventGroupCreate();

    xTaskCreate(bt_speaker_task, "bt_speaker_task", 2048, NULL, 5, NULL);
    xTaskCreate(mp3_player_task, "mp3_player_task", 8192, NULL, 5, NULL);
    xTaskCreate(oled_display_task, "oled_display_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_indicator_task, "led_indicator_task", 1024, NULL, 5, NULL);
}
