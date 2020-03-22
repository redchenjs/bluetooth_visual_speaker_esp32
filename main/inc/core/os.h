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
    OS_PWR_SLEEP_BIT      = BIT0,
    OS_PWR_RESTART_BIT    = BIT1,

    BT_A2DP_IDLE_BIT      = BIT2,
    BT_A2DP_DATA_BIT      = BIT3,

    BT_SPP_IDLE_BIT       = BIT4,
    BT_OTA_LOCKED_BIT     = BIT5,

    BLE_GATTS_IDLE_BIT    = BIT6,

    VFX_RELOAD_BIT        = BIT7,
    VFX_FFT_NULL_BIT      = BIT8,

    KEY_SCAN_RUN_BIT      = BIT9,

    AUDIO_INPUT_RUN_BIT   = BIT10,
    AUDIO_INPUT_FFT_BIT   = BIT11,

    AUDIO_RENDER_CLR_BIT  = BIT12,

    AUDIO_PLAYER_RUN_BIT  = BIT13,
    AUDIO_PLAYER_IDLE_BIT = BIT14,
    AUDIO_PLAYER_EXIT_BIT = BIT15,
} user_event_group_bits_t;

extern EventGroupHandle_t user_event_group;

extern void os_power_sleep_wait(EventBits_t bits);
extern void os_power_restart_wait(EventBits_t bits);

extern void os_init(void);

#endif /* INC_CORE_OS_H_ */
