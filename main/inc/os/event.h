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
    VFX_RELOAD_BIT  = BIT0,
    AUDIO_READY_BIT = BIT1
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

#endif /* INC_OS_EVENT_H_ */
