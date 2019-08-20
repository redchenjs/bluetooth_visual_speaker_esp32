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
    BT_A2D_RUN_BIT       = BIT0,
    BT_SPP_RUN_BIT       = BIT1,
    VFX_RELOAD_BIT       = BIT2,
    KEY_SCAN_RUN_BIT     = BIT3,
    AUDIO_MP3_RUN_BIT    = BIT4,
    AUDIO_MP3_IDLE_BIT   = BIT5,
    AUDIO_INPUT_RUN_BIT  = BIT6,
    AUDIO_INPUT_LOOP_BIT = BIT7
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

#ifdef CONFIG_ENABLE_WAKEUP_KEY
extern void os_enter_sleep_mode(void);
#endif

extern void os_core_init(void);

#endif /* INC_OS_CORE_H_ */
