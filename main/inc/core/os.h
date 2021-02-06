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
    OS_PWR_DUMMY_BIT = 0x00,
    OS_PWR_RESET_BIT = BIT0,
    OS_PWR_SLEEP_BIT = BIT1,

    BT_A2DP_IDLE_BIT = BIT2,

    VFX_RLD_MODE_BIT = BIT3,
    VFX_FFT_IDLE_BIT = BIT4,

    KEY_SCAN_RUN_BIT = BIT5,
    KEY_SCAN_CLR_BIT = BIT6,

    BLE_GATTS_IDLE_BIT = BIT7,
    BLE_GATTS_LOCK_BIT = BIT8,

    AUDIO_INPUT_RUN_BIT = BIT9,
    AUDIO_INPUT_FFT_BIT = BIT10,

    AUDIO_PLAYER_RUN_BIT  = BIT11,
    AUDIO_PLAYER_IDLE_BIT = BIT12
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

extern void os_pwr_reset_wait(EventBits_t bits);
extern void os_pwr_sleep_wait(EventBits_t bits);

extern void os_init(void);

#endif /* INC_CORE_OS_H_ */
