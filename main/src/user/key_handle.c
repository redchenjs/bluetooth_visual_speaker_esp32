/*
 * key_handle.c
 *
 *  Created on: 2019-07-06 10:35
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "core/os.h"
#include "user/vfx.h"
#include "user/ain.h"
#include "user/bt_av.h"
#include "user/ble_gatts.h"
#include "user/audio_player.h"

#ifdef CONFIG_ENABLE_SLEEP_KEY
void sleep_key_handle(void)
{
    xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);

#ifdef CONFIG_ENABLE_VFX
    vfx_config_t *vfx = vfx_get_conf();
    vfx->mode = VFX_MODE_IDX_OFF;
    vfx_set_conf(vfx);
#endif
#ifndef CONFIG_AUDIO_INPUT_NONE
    ain_set_mode(0);
#endif
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    audio_player_play_file(3);
#endif

    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if (!(uxBits & BT_A2DP_IDLE_BIT)) {
        esp_a2d_sink_disconnect(a2d_remote_bda);
    }
#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
    if (!(uxBits & BLE_GATTS_IDLE_BIT)) {
        esp_ble_gatts_close(gatts_profile_tbl[PROFILE_IDX_OTA].gatts_if,
                            gatts_profile_tbl[PROFILE_IDX_OTA].conn_id);
    }
#endif

    os_power_sleep_wait(
        BT_A2DP_IDLE_BIT | BLE_GATTS_IDLE_BIT
#ifdef CONFIG_ENABLE_AUDIO_PROMPT
        | AUDIO_PLAYER_IDLE_BIT
#endif
    );
}
#endif // CONFIG_ENABLE_SLEEP_KEY
