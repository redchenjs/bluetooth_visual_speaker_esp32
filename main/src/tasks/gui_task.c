/*
 * gui_task.c
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "gfx.h"
#include "esp_log.h"

#include "system/event.h"
#include "tasks/gui_task.h"

#define TAG "gui_task"

static const uint8_t *img_file_ptr[][2] = {
#if defined(CONFIG_OLED_PANEL_SSD1331)
                                            {ani0_96x64_gif_ptr, ani0_96x64_gif_end}, // "Bluetooth"
                                            {ani1_96x64_gif_ptr, ani1_96x64_gif_end}, // "Standby"
                                            {ani2_96x64_gif_ptr, ani2_96x64_gif_end}, // "Pause"
                                            {ani3_96x64_gif_ptr, ani3_96x64_gif_end}  // "Playing"
#elif defined(CONFIG_OLED_PANEL_SSD1351)
                                            {ani0_128x128_gif_ptr, ani0_128x128_gif_end}, // "Bluetooth"
                                            {ani1_128x128_gif_ptr, ani1_128x128_gif_end}, // "Standby"
                                            {ani2_128x128_gif_ptr, ani2_128x128_gif_end}, // "Pause"
                                            {ani3_128x128_gif_ptr, ani3_128x128_gif_end}  // "Playing"
#endif
                                         };
uint8_t img_file_index = 0;

void gui_show_image(uint8_t filename_index)
{
#if defined(CONFIG_OLED_PANEL_SSD1331) || defined(CONFIG_OLED_PANEL_SSD1351)
    if (filename_index >= (sizeof(img_file_ptr) / 2)) {
        ESP_LOGE(TAG, "invalid filename index");
        return;
    }
    img_file_index = filename_index;
    xEventGroupSetBits(task_event_group, GUI_RELOAD_BIT);
#endif
}

void gui_task(void *pvParameter)
{
    gfxInit();

    while (1) {
        gdispImage gfx_image;
        if (!(gdispImageOpenMemory(&gfx_image, img_file_ptr[img_file_index][0]) & GDISP_IMAGE_ERR_UNRECOVERABLE)) {
            gdispImageSetBgColor(&gfx_image, White);
            while (1) {
                if (xEventGroupGetBits(task_event_group) & GUI_RELOAD_BIT) {
                    xEventGroupClearBits(task_event_group, GUI_RELOAD_BIT);
                    break;
                }
                if (gdispImageDraw(&gfx_image, 0, 0, gfx_image.width, gfx_image.height, 0, 0) != GDISP_IMAGE_ERR_OK) {
                    break;
                }
                delaytime_t delay = gdispImageNext(&gfx_image);
                if (delay == TIME_INFINITE) {
                    break;
                }
                if (delay != TIME_IMMEDIATE) {
                    gfxSleepMilliseconds(delay);
                }
            }
            gdispImageClose(&gfx_image);
        } else {
            break;
        }
    }
    ESP_LOGE(TAG, "task failed, rebooting...");
    esp_restart();
}
