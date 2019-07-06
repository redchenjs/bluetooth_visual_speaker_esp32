/*
 * core.h
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_OS_CORE_H_
#define INC_OS_CORE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef enum user_event_group_bits {
    VFX_RELOAD_BIT     = BIT0,
    KEY_SCAN_RUN_BIT   = BIT1,
    AUDIO_MP3_RUN_BIT  = BIT2,
    AUDIO_MP3_IDLE_BIT = BIT3
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

#ifdef CONFIG_ENABLE_WAKEUP_KEY
extern void os_enter_sleep_mode(void);
#endif

extern void core_init(void);

#endif /* INC_OS_CORE_H_ */
