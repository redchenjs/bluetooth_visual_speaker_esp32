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
    VFX_FFT_EXEC_BIT     = BIT1,
    VFX_FFT_FULL_BIT     = BIT2,
    KEY_SCAN_RUN_BIT     = BIT3,
    BT_OTA_LOCKED_BIT    = BIT4,
    AUDIO_MP3_RUN_BIT    = BIT5,
    AUDIO_MP3_IDLE_BIT   = BIT6,
    AUDIO_INPUT_RUN_BIT  = BIT7,
    AUDIO_INPUT_LOOP_BIT = BIT8,
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

#ifdef CONFIG_ENABLE_WAKEUP_KEY
extern void os_enter_sleep_mode(void);
#endif

extern void os_init(void);

#endif /* INC_CORE_OS_H_ */
