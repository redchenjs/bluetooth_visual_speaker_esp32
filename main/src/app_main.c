/*
 * app_main.c
 *
 *  Created on: 2018-03-11 15:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "core/os.h"
#include "core/app.h"

#include "chip/bt.h"
#include "chip/nvs.h"
#include "chip/spi.h"
#include "chip/i2s.h"

#include "user/led.h"
#include "user/vfx.h"
#include "user/key.h"
#include "user/ain.h"
#include "user/bt_app.h"
#include "user/ble_app.h"
#include "user/audio_player.h"
#include "user/audio_render.h"

static void core_init(void)
{
    app_print_info();

    os_init();
}

static void chip_init(void)
{
    nvs_init();

    bt_init();

    i2s_output_init();

#ifndef CONFIG_AUDIO_INPUT_NONE
    i2s_input_init();
#endif

#ifdef CONFIG_ENABLE_VFX
    hspi_init();
#endif
}

static void board_init(void) {}

static void user_init(void)
{
#ifdef CONFIG_ENABLE_VFX
    vfx_init();
#endif

#ifdef CONFIG_ENABLE_LED
    led_init();
#endif

#ifdef CONFIG_ENABLE_SLEEP_KEY
    key_init();
#endif

#ifndef CONFIG_AUDIO_INPUT_NONE
    ain_init();
#endif

#ifdef CONFIG_ENABLE_AUDIO_PROMPT
    audio_player_init();
#endif

#ifdef CONFIG_ENABLE_BLE_CONTROL_IF
    ble_app_init();
#endif

    audio_render_init();

    bt_app_init();
}

int app_main(void)
{
    core_init();

    chip_init();

    board_init();

    user_init();

    return 0;
}
