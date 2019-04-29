/*
 * event.h
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_OS_EVENT_H_
#define INC_OS_EVENT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef enum user_event_group_bits {
    AUDIO_RUN_BIT  = BIT0,
    VFX_RELOAD_BIT = BIT1
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

extern void event_init(void);

#endif /* INC_OS_EVENT_H_ */
