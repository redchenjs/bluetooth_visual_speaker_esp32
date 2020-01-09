/*
 * key_handle.c
 *
 *  Created on: 2019-07-06 10:35
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"
#include "esp_sleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "core/os.h"
#include "user/vfx.h"
#include "user/bt_av.h"
#include "user/bt_spp.h"
#include "user/ble_gatts.h"
#include "user/audio_input.h"
#include "user/audio_player.h"

#ifdef CONFIG_ENABLE_SLEEP_KEY
void sleep_key_handle(void)
{
    xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);

#ifdef CONFIG_ENABLE_VFX
    struct vfx_conf *vfx = vfx_get_conf();
    vfx->mode = 0;
    vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
    audio_input_set_mode(0);
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    audio_player_play_file(3);
#endif

    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if (!(uxBits & BT_A2DP_IDLE_BIT)) {
        esp_a2d_sink_disconnect(a2d_remote_bda);
    }
#ifdef CONFIG_ENABLE_OTA_OVER_SPP
    if (!(uxBits & BT_SPP_IDLE_BIT)) {
        esp_spp_disconnect(spp_conn_handle);
    }
#endif
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
    if (!(uxBits & BLE_GATTS_IDLE_BIT)) {
        esp_ble_gatts_close(gl_profile_tab[PROFILE_A_APP_ID].gatts_if,
                            gl_profile_tab[PROFILE_A_APP_ID].conn_id);
    }
#endif

    os_power_sleep_wait(BT_SPP_IDLE_BIT | BT_A2DP_IDLE_BIT | BLE_GATTS_IDLE_BIT | AUDIO_PLAYER_IDLE_BIT);
}
#endif // CONFIG_ENABLE_SLEEP_KEY
