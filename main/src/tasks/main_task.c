/*
 * main_task.c
 *
 *  Created on: 2018-02-16 18:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks/bt_speaker.h"
#include "tasks/led_indicator.h"

void main_task(void)
{
    xTaskCreate(bt_speaker_task, "bt_speaker_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_indicator_task, "led_indicator_task", 1024, NULL, 1, NULL);
}
