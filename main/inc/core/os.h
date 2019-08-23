/*
 * os.h
 *
 *  Created on: 2018-03-04 20:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CORE_OS_H_
#define INC_CORE_OS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef enum user_event_group_bits {
    VFX_RELOAD_BIT       = BIT0,
    KEY_SCAN_RUN_BIT     = BIT1,
    BT_OTA_LOCKED_BIT    = BIT2,
    AUDIO_MP3_RUN_BIT    = BIT3,
    AUDIO_MP3_IDLE_BIT   = BIT4,
    AUDIO_INPUT_RUN_BIT  = BIT5,
    AUDIO_INPUT_LOOP_BIT = BIT6
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

#ifdef CONFIG_ENABLE_WAKEUP_KEY
extern void os_enter_sleep_mode(void);
#endif

extern void os_init(void);

#endif /* INC_CORE_OS_H_ */
