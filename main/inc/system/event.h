/*
 * event.c
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_SYSTEM_EVENT_H_
#define INC_SYSTEM_EVENT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef enum daemon_event_group_bits {
    GUI_DAEMON_RELOAD_BIT  = BIT0,
    AUDIO_DAEMON_READY_BIT = BIT1
} daemon_event_group_bits_t;

extern EventGroupHandle_t daemon_event_group;

extern void event_init(void);

#endif /* INC_SYSTEM_EVENT_H_ */
