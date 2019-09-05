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
#include "user/bt_app_av.h"
#include "user/audio_mp3.h"

#ifdef CONFIG_ENABLE_SLEEP_KEY
void key_sleep_handle(void)
{
    xEventGroupClearBits(user_event_group, KEY_SCAN_RUN_BIT);

    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if (!(uxBits & BT_A2DP_IDLE_BIT)) {
        esp_a2d_sink_disconnect(a2d_remote_bda);
    }

    vfx_set_mode(0);

    audio_mp3_play_file(3);
    xEventGroupWaitBits(
        user_event_group,
        AUDIO_MP3_IDLE_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    os_enter_sleep_mode();
}
#endif // CONFIG_ENABLE_SLEEP_KEY
