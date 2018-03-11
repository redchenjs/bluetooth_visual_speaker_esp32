/*
 * event.c
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_SYSTEM_EVENT_C_
#define INC_SYSTEM_EVENT_C_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

enum task_event_group_bits {
    MP3_PLAYER_READY_BIT = BIT0,
    GUI_RELOAD_BIT       = BIT1
};

extern EventGroupHandle_t task_event_group;

extern void event_init(void);

#endif /* INC_SYSTEM_EVENT_C_ */
