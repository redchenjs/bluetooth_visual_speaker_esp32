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
    VFX_RELOAD_BIT        = BIT0,
    VFX_FFT_EXEC_BIT      = BIT1,
    VFX_FFT_FULL_BIT      = BIT2,

    NFC_APP_RUN_BIT       = BIT3,
    KEY_SCAN_RUN_BIT      = BIT4,

    BT_SPP_IDLE_BIT       = BIT5,
    BT_A2DP_IDLE_BIT      = BIT6,
    BT_OTA_LOCKED_BIT     = BIT7,
    BLE_GATTS_IDLE_BIT    = BIT8,

    OS_PWR_SLEEP_BIT      = BIT9,
    OS_PWR_RESTART_BIT    = BIT10,

    AUDIO_INPUT_RUN_BIT   = BIT11,
    AUDIO_PLAYER_RUN_BIT  = BIT12,
    AUDIO_PLAYER_IDLE_BIT = BIT13,
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

extern void os_power_sleep_wait(EventBits_t bits);
extern void os_power_restart_wait(EventBits_t bits);

extern void os_init(void);

#endif /* INC_CORE_OS_H_ */
