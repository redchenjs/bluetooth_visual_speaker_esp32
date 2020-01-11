/*
 * vfx.c
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <math.h>
#include <string.h>

#include "esp_log.h"

#include "fft.h"
#include "gfx.h"

#include "core/os.h"
#include "core/app.h"
#include "user/vfx.h"
#include "user/vfx_core.h"
#include "user/vfx_bitmap.h"
#include "user/audio_input.h"

#define TAG "vfx"

#define VFX_PERIOD GDISP_NEED_TIMERFLUSH

static struct vfx_conf vfx = {
    .mode = DEFAULT_VFX_MODE,
    .scale_factor = DEFAULT_VFX_SCALE_FACTOR,
    .lightness = DEFAULT_VFX_LIGHTNESS,
    .backlight = DEFAULT_VFX_BACKLIGHT,
};

GDisplay *vfx_gdisp = NULL;
fft_config_t *vfx_fft_plan = NULL;

static coord_t vfx_disp_width = 0;
static coord_t vfx_disp_height = 0;

#ifdef CONFIG_SCREEN_PANEL_OUTPUT_VFX
static const char *img_file_ptr[][2] = {
    #ifdef CONFIG_VFX_OUTPUT_ST7735
        {ani0_160x80_gif_ptr, ani0_160x80_gif_end},     // "Nyan Cat"
        {ani1_160x80_gif_ptr, ani1_160x80_gif_end},     // "bilibili"
    #elif defined(CONFIG_VFX_OUTPUT_ST7789)
        {ani0_240x135_gif_ptr, ani0_240x135_gif_end},   // "Nyan Cat"
        {ani1_240x135_gif_ptr, ani1_240x135_gif_end},   // "bilibili"
    #endif
};
#endif

static void vfx_task(void *pvParameter)
{
    portTickType xLastWakeTime;

    gfxInit();

    vfx_gdisp = gdispGetDisplay(0);
    vfx_disp_width = gdispGGetWidth(vfx_gdisp);
    vfx_disp_height = gdispGGetHeight(vfx_gdisp);

    ESP_LOGI(TAG, "started.");

    while (1) {
        switch (vfx.mode) {
#ifdef CONFIG_SCREEN_PANEL_OUTPUT_VFX
        // LCD Output
        case 0x00:
        case 0x01: {   // 動態貼圖
            gdispImage gfx_image;

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            if (!(gdispImageOpenMemory(&gfx_image, img_file_ptr[vfx.mode][0]) & GDISP_IMAGE_ERR_UNRECOVERABLE)) {
                gdispImageSetBgColor(&gfx_image, Black);

                while (1) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                        break;
                    }

                    if (gdispImageDraw(&gfx_image, 0, 0, gfx_image.width, gfx_image.height, 0, 0) != GDISP_IMAGE_ERR_OK) {
                        ESP_LOGE(TAG, "failed to draw image: %u", vfx.mode);
                        vfx.mode = VFX_MODE_OFF;
                        break;
                    }

                    delaytime_t delay = gdispImageNext(&gfx_image);
                    if (delay == TIME_INFINITE) {
                        vfx.mode = VFX_MODE_PAUSE;
                        break;
                    }

                    if (delay != TIME_IMMEDIATE) {
                        vTaskDelayUntil(&xLastWakeTime, delay / portTICK_RATE_MS);
                    }
                }

                gdispImageClose(&gfx_image);
            } else {
                ESP_LOGE(TAG, "failed to open image: %u", vfx.mode);
                vfx.mode = VFX_MODE_OFF;
                break;
            }
            break;
        }
        case 0x0E: {   // 音樂頻譜-漸變-對數
            uint8_t  color_cnt = 0;
            uint16_t color_tmp = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float  fft_amp[64] = {0};
            int8_t fft_out[64] = {0};
            uint16_t center_y = vfx_disp_height % 2 ? vfx_disp_height / 2 : vfx_disp_height / 2 - 1;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = log10(fft_amp[0]) / (65536 / vfx_disp_height) * vfx.scale_factor * 64 / 2;
                    if (fft_out[0] > center_y) {
                        fft_out[0] = center_y;
                    } else if (fft_out[0] < 0) {
                        fft_out[0] = 0;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = log10(fft_amp[k]) / (65536 / vfx_disp_height) * vfx.scale_factor * 64 / 2;
                        if (fft_out[k] > center_y) {
                            fft_out[k] = center_y;
                        } else if (fft_out[k] < 0) {
                            fft_out[k] = 0;
                        }
                    }
                }

                color_tmp = color_h;
                for (uint16_t i=0; i<vfx_disp_width; i++) {
                    uint32_t pixel_color = vfx_read_color_from_table(color_h, color_l);

#if defined(CONFIG_VFX_OUTPUT_ST7735)
                    uint16_t clear_x  = i * 3;
                    uint16_t clear_cx = 3;
                    uint16_t clear_u_y = 0;
                    uint16_t clear_d_y = center_y + fft_out[i] + 2;
                    uint16_t clear_cy  = center_y - fft_out[i];

                    uint16_t fill_x  = i * 3;
                    uint16_t fill_cx = 3;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 2;
#else
                    uint16_t clear_x  = i * 4;
                    uint16_t clear_cx = 4;
                    uint16_t clear_u_y = 0;
                    uint16_t clear_d_y = center_y + fft_out[i] + 1;
                    uint16_t clear_cy  = center_y - fft_out[i];

                    uint16_t fill_x  = i * 4;
                    uint16_t fill_cx = 4;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 1;
#endif

                    gdispGFillArea(vfx_gdisp, clear_x, clear_u_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, clear_x, clear_d_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if (color_h++ == 511) {
                        color_h = 0;
                    }
                }

                if (++color_cnt % (16 / VFX_PERIOD) == 0) {
                    color_h = ++color_tmp;
                } else {
                    color_h = color_tmp;
                }

                if (color_h > 511) {
                    color_h = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x0F: {   // 音樂頻譜-彩虹-對數
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float  fft_amp[64] = {0};
            int8_t fft_out[64] = {0};
            uint16_t center_y = vfx_disp_height % 2 ? vfx_disp_height / 2 : vfx_disp_height / 2 - 1;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = log10(fft_amp[0]) / (65536 / vfx_disp_height) * vfx.scale_factor * 64 / 2;
                    if (fft_out[0] > center_y) {
                        fft_out[0] = center_y;
                    } else if (fft_out[0] < 0) {
                        fft_out[0] = 0;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = log10(fft_amp[k]) / (65536 / vfx_disp_height) * vfx.scale_factor * 64 / 2;
                        if (fft_out[k] > center_y) {
                            fft_out[k] = center_y;
                        } else if (fft_out[k] < 0) {
                            fft_out[k] = 0;
                        }
                    }
                }

                color_h = 511;
                for (uint16_t i=0; i<vfx_disp_width; i++) {
                    uint32_t pixel_color = vfx_read_color_from_table(color_h, color_l);

#if defined(CONFIG_VFX_OUTPUT_ST7735)
                    uint16_t clear_x  = i * 3;
                    uint16_t clear_cx = 3;
                    uint16_t clear_u_y = 0;
                    uint16_t clear_d_y = center_y + fft_out[i] + 2;
                    uint16_t clear_cy  = center_y - fft_out[i];

                    uint16_t fill_x  = i * 3;
                    uint16_t fill_cx = 3;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 2;
#else
                    uint16_t clear_x  = i * 4;
                    uint16_t clear_cx = 4;
                    uint16_t clear_u_y = 0;
                    uint16_t clear_d_y = center_y + fft_out[i] + 1;
                    uint16_t clear_cy  = center_y - fft_out[i];

                    uint16_t fill_x  = i * 4;
                    uint16_t fill_cx = 4;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 1;
#endif

                    gdispGFillArea(vfx_gdisp, clear_x, clear_u_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, clear_x, clear_d_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if ((color_h -= 8) == 7) {
                        color_h = 511;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x10: {   // 音樂頻譜-漸變-線性
            uint8_t  color_cnt = 0;
            uint16_t color_tmp = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float   fft_amp[64] = {0};
            uint8_t fft_out[64] = {0};

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = fft_amp[0] / (65536 / vfx_disp_height) * vfx.scale_factor;
                    if (fft_out[0] > vfx_disp_height) {
                        fft_out[0] = vfx_disp_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = fft_amp[k] / (65536 / vfx_disp_height) * vfx.scale_factor;
                        if (fft_out[k] > vfx_disp_height) {
                            fft_out[k] = vfx_disp_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_tmp = color_h;
                for (uint16_t i=0; i<vfx_disp_width; i++) {
                    uint32_t pixel_color = vfx_read_color_from_table(color_h, color_l);

#if defined(CONFIG_VFX_OUTPUT_ST7735)
                    uint16_t clear_x  = i * 3;
                    uint16_t clear_cx = 3;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height - fft_out[i];

                    uint16_t fill_x  = i * 3;
                    uint16_t fill_cx = 3;
                    uint16_t fill_y  = vfx_disp_height - fft_out[i];
                    uint16_t fill_cy = fft_out[i];
#else
                    uint16_t clear_x  = i * 4;
                    uint16_t clear_cx = 4;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height - fft_out[i];

                    uint16_t fill_x  = i * 4;
                    uint16_t fill_cx = 4;
                    uint16_t fill_y  = vfx_disp_height - fft_out[i];
                    uint16_t fill_cy = fft_out[i];
#endif

                    gdispGFillArea(vfx_gdisp, clear_x, clear_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if (color_h++ == 511) {
                        color_h = 0;
                    }
                }

                if (++color_cnt % (16 / VFX_PERIOD) == 0) {
                    color_h = ++color_tmp;
                } else {
                    color_h = color_tmp;
                }

                if (color_h > 511) {
                    color_h = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x11: {   // 音樂頻譜-彩虹-線性
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float   fft_amp[64] = {0};
            uint8_t fft_out[64] = {0};

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = fft_amp[0] / (65536 / vfx_disp_height) * vfx.scale_factor;
                    if (fft_out[0] > vfx_disp_height) {
                        fft_out[0] = vfx_disp_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = fft_amp[k] / (65536 / vfx_disp_height) * vfx.scale_factor;
                        if (fft_out[k] > vfx_disp_height) {
                            fft_out[k] = vfx_disp_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_h = 511;
                for (uint16_t i=0; i<vfx_disp_width; i++) {
                    uint32_t pixel_color = vfx_read_color_from_table(color_h, color_l);

#if defined(CONFIG_VFX_OUTPUT_ST7735)
                    uint16_t clear_x  = i * 3;
                    uint16_t clear_cx = 3;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height - fft_out[i];

                    uint16_t fill_x  = i * 3;
                    uint16_t fill_cx = 3;
                    uint16_t fill_y  = vfx_disp_height - fft_out[i];
                    uint16_t fill_cy = fft_out[i];
#else
                    uint16_t clear_x  = i * 4;
                    uint16_t clear_cx = 4;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height - fft_out[i];

                    uint16_t fill_x  = i * 4;
                    uint16_t fill_cx = 4;
                    uint16_t fill_y  = vfx_disp_height - fft_out[i];
                    uint16_t fill_cy = fft_out[i];
#endif

                    gdispGFillArea(vfx_gdisp, clear_x, clear_y, clear_cx, clear_cy, 0x000000);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if ((color_h -= 8) == 7) {
                        color_h = 511;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
#else
        // Light Cube Output
        case 0x01: {   // 漸變-點
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                while (1) {
                    vfx_draw_pixel(x, y, z, color_h, color_l);

                    if (x++ == 7) {
                        x = 0;
                        if (y++ == 7) {
                            y = 0;
                            if (z++ == 7) {
                                z = 0;
                                break;
                            }
                        }
                    }

                    if (color_h++ == 511) {
                        color_h = 0;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }
            break;
        }
        case 0x02: {   // 漸變-面
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_tmp = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                color_tmp = color_h;
                while (1) {
                    for (uint8_t i=0; i<8; i++) {
                        vfx_draw_pixel(x, y, i, color_h, color_l);
                    }

                    if (x++ == 7) {
                        x = 0;
                        if (y++ == 7) {
                            y = 0;
                            break;
                        }
                    }

                    if (color_h++ == 511) {
                        color_h = 0;
                    }
                }

                color_h = ++color_tmp;
                if (color_h > 511) {
                    color_h = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }
            break;
        }
        case 0x03: {   // 漸變-體
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                vfx_fill_cube(0, 0, 0, 8, 8, 8, color_h, color_l);

                if (color_h++ == 511) {
                    color_h = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }
            break;
        }
        case 0x04: {   // 呼吸-體
            uint8_t scale_dir = 0;
            uint8_t color_cnt = 0;
            uint16_t color_h = 0;
            uint16_t color_l = 0;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                vfx_fill_cube(0, 0, 0, 8, 8, 8, color_h, color_l);

                if (scale_dir == 0) {   // 暗->明
                    if (color_l++ == vfx.lightness) {
                        color_l = vfx.lightness;
                        scale_dir = 1;
                    }
                } else {                // 明->暗
                    if (color_l-- == 0) {
                        color_l = 0;
                        scale_dir = 0;
                    }
                }
                if (++color_cnt % 16 == 0) {
                    if (color_h++ == 511) {
                        color_h = 0;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }
            break;
        }
        case 0x05: {   // 星空-紫紅
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_h[512] = {0};
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            for (uint16_t i=0; i<=511; i++) {
                led_idx[i] = i;
                color_h[i] = i % 80;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = led_idx[rnd];
                led_idx[rnd] = led_idx[i];
                led_idx[i] = tmp;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = color_h[rnd];
                color_h[rnd] = color_h[i];
                color_h[i] = tmp;
            }

            int16_t idx_base = -led_num;
            while (1) {
                for (uint16_t i=0; i<=color_l; i++) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                        goto loop_break;
                    }

                    if (idx_base >= 0) {
                        x = (led_idx[idx_base] % 64) % 8;
                        y = (led_idx[idx_base] % 64) / 8;
                        z = led_idx[idx_base] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base], color_l - i);
                    }

                    if ((idx_base + led_num) <= 511) {
                        x = (led_idx[idx_base + led_num] % 64) % 8;
                        y = (led_idx[idx_base + led_num] % 64) / 8;
                        z = led_idx[idx_base + led_num] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base + led_num], i);
                    }

                    vTaskDelayUntil(&xLastWakeTime, 8 / portTICK_RATE_MS);
                }
                if (idx_base++ == 511) {
                    idx_base = -led_num;
                }
            }
            break;
        }
        case 0x06: {   // 星空-靑藍
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_h[512] = {0};
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            for (uint16_t i=0; i<=511; i++) {
                led_idx[i] = i;
                color_h[i] = i % 80 + 170;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = led_idx[rnd];
                led_idx[rnd] = led_idx[i];
                led_idx[i] = tmp;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = color_h[rnd];
                color_h[rnd] = color_h[i];
                color_h[i] = tmp;
            }

            int16_t idx_base = -led_num;
            while (1) {
                for (uint16_t i=0; i<=color_l; i++) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                        goto loop_break;
                    }

                    if (idx_base >= 0) {
                        x = (led_idx[idx_base] % 64) % 8;
                        y = (led_idx[idx_base] % 64) / 8;
                        z = led_idx[idx_base] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base], color_l - i);
                    }

                    if ((idx_base + led_num) <= 511) {
                        x = (led_idx[idx_base + led_num] % 64) % 8;
                        y = (led_idx[idx_base + led_num] % 64) / 8;
                        z = led_idx[idx_base + led_num] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base + led_num], i);
                    }

                    vTaskDelayUntil(&xLastWakeTime, 8 / portTICK_RATE_MS);
                }
                if (idx_base++ == 511) {
                    idx_base = -led_num;
                }
            }
            break;
        }
        case 0x07: {   // 星空-黃綠
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_h[512] = {0};
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            for (uint16_t i=0; i<=511; i++) {
                led_idx[i] = i;
                color_h[i] = i % 85 + 345;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = led_idx[rnd];
                led_idx[rnd] = led_idx[i];
                led_idx[i] = tmp;
            }

            for (uint16_t i=0; i<=511; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = color_h[rnd];
                color_h[rnd] = color_h[i];
                color_h[i] = tmp;
            }

            int16_t idx_base = -led_num;
            while (1) {
                for (uint16_t i=0; i<=color_l; i++) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                        goto loop_break;
                    }

                    if (idx_base >= 0) {
                        x = (led_idx[idx_base] % 64) % 8;
                        y = (led_idx[idx_base] % 64) / 8;
                        z = led_idx[idx_base] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base], color_l - i);
                    }

                    if ((idx_base + led_num) <= 511) {
                        x = (led_idx[idx_base + led_num] % 64) % 8;
                        y = (led_idx[idx_base + led_num] % 64) / 8;
                        z = led_idx[idx_base + led_num] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base + led_num], i);
                    }

                    vTaskDelayUntil(&xLastWakeTime, 8 / portTICK_RATE_MS);
                }
                if (idx_base++ == 511) {
                    idx_base = -led_num;
                }
            }
            break;
        }
        case 0x08: {   // 數字-固定
            uint16_t num = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                vfx_draw_layer_number(num, 2, color_h, color_l);
                vfx_draw_layer_number(num, 3, color_h, color_l);
                vfx_draw_layer_number(num, 4, color_h, color_l);
                vfx_draw_layer_number(num, 5, color_h, color_l);

                color_h += 8;
                if (color_h > 511) {
                    color_h = 0;
                }

                if (num++ == 9) {
                    num = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_RATE_MS);
            }
            break;
        }
        case 0x09: {   // 數字-滾動
            uint16_t num = 0;
            uint16_t layer0 = 0;
            uint16_t layer1 = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                for (uint8_t i=layer0; i<=layer1; i++) {
                    vfx_draw_layer_number(num, i, color_h, color_l);
                }

                if (layer1 != 7 && layer0 != 0) {
                    vfx_draw_layer_number(num, layer0, 0, 0);

                    layer1++;
                    layer0++;
                } else if (layer1 == 7) {
                    if (layer0++ == 7) {
                        vfx_draw_layer_number(num, 7, 0, 0);

                        layer0 = 0;
                        layer1 = 0;

                        if (num++ == 9) {
                            num = 0;
                        }
                    } else {
                        vfx_draw_layer_number(num, layer0 - 1, 0, 0);
                    }
                } else {
                    if ((layer1 - layer0) != 4) {
                        layer1++;
                    } else {
                        vfx_draw_layer_number(num, 0, 0, 0);

                        layer1++;
                        layer0++;
                    }
                }

                if (color_h++ == 511) {
                    color_h = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, 80 / portTICK_RATE_MS);
            }
            break;
        }
        case 0x0A: {   // 跳躍飛毯
            uint16_t frame_idx = 0;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                vfx_draw_cube_bitmap(vfx_bitmap_wave[frame_idx], vfx.lightness);

                if (frame_idx++ == 44) {
                    frame_idx = 8;
                }

                vTaskDelayUntil(&xLastWakeTime, 16 / portTICK_RATE_MS);
            }
            break;
        }
        case 0x0B: {   // 旋轉曲面-正
            uint16_t frame_pre = 0;
            uint16_t frame_idx = 0;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                frame_pre = frame_idx;
                for (uint8_t i=0; i<8; i++) {
                    vfx_draw_layer_bitmap(i, vfx_bitmap_line[frame_idx], vfx.lightness);

                    if (frame_idx++ == 27) {
                        frame_idx = 0;
                    }
                }

                if (frame_pre == 27) {
                    frame_idx = 0;
                } else {
                    frame_idx = frame_pre + 1;
                }

                vTaskDelayUntil(&xLastWakeTime, 40 / portTICK_RATE_MS);
            }
            break;
        }
        case 0x0C: {   // 旋轉曲面-反
            uint16_t frame_pre = 0;
            uint16_t frame_idx = 0;

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    break;
                }

                frame_pre = frame_idx;
                for (uint8_t i=0; i<8; i++) {
                    vfx_draw_layer_bitmap(i, vfx_bitmap_line[frame_idx], vfx.lightness);

                    if (frame_idx-- == 0) {
                        frame_idx = 27;
                    }
                }

                if (frame_pre == 0) {
                    frame_idx = 27;
                } else {
                    frame_idx = frame_pre - 1;
                }

                vTaskDelayUntil(&xLastWakeTime, 40 / portTICK_RATE_MS);
            }
            break;
        }
        case 0x0D: {   // 音樂噴泉-靜態-對數
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float  fft_amp[64] = {0};
            int8_t fft_out[64] = {0};
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = log10(fft_amp[0]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = log10(fft_amp[k]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_h = 511;
                for (uint16_t i=0; i<canvas_width; i++) {
                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h, color_l);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    if ((color_h -= 8) == 7) {
                        color_h = 511;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x0E: {   // 音樂噴泉-漸變-對數
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t  color_cnt = 0;
            uint16_t color_tmp = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float  fft_amp[64] = {0};
            int8_t fft_out[64] = {0};
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = log10(fft_amp[0]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = log10(fft_amp[k]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_h = color_tmp;
                for (uint16_t i=0; i<canvas_width; i++) {
                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h, color_l);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    color_h += 8;
                    if (color_h > 511) {
                        color_h = 0;
                    }
                }

                if (++color_cnt % (128 / VFX_PERIOD) == 0) {
                    color_tmp += 8;
                    if (color_tmp > 511) {
                        color_tmp = 0;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x0F: {   // 音樂噴泉-螺旋-對數
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_flg = 0;
            uint8_t color_cnt = 0;
            uint16_t color_h[64] = {0};
            uint16_t color_l[64] = {vfx.lightness};
            float  fft_amp[64] = {0};
            int8_t fft_out[64] = {0};
            const uint8_t led_idx_table[][64] = {
                {
                    3, 4, 4, 3, 2, 2, 2, 3, 4, 5, 5, 5, 5, 4, 3, 2,
                    1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 5,
                    4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
                    6, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0,
                },
                {
                    3, 3, 4, 4, 4, 3, 2, 2, 2, 2, 3, 4, 5, 5, 5, 5,
                    5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6,
                    6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7,
                }
            };
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            for (uint16_t i=0; i<64; i++) {
                color_h[i] = i * 8;
                color_l[i] = vfx.lightness;
            }

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = log10(fft_amp[0]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = log10(fft_amp[k]) / (65536 / canvas_height) * vfx.scale_factor * 64;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                for (uint16_t i=0; i<canvas_width; i++) {
                    x = led_idx_table[0][i];
                    y = led_idx_table[1][i];

                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h[i], color_l[i]);

                    if (color_flg) {
                        if (color_h[i]-- == 0) {
                            color_h[i] = 511;
                        }
                    }
                }

                if (++color_cnt % (32 / VFX_PERIOD) == 0) {
                    color_flg = 1;
                } else {
                    color_flg = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x10: {   // 音樂噴泉-靜態-線性
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float   fft_amp[64] = {0};
            uint8_t fft_out[64] = {0};
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = fft_amp[0] / (65536 / canvas_height) * vfx.scale_factor;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = fft_amp[k] / (65536 / canvas_height) * vfx.scale_factor;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_h = 511;
                for (uint16_t i=0; i<canvas_width; i++) {
                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h, color_l);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    if ((color_h -= 8) == 7) {
                        color_h = 511;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x11: {   // 音樂噴泉-漸變-線性
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t  color_cnt = 0;
            uint16_t color_tmp = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            float   fft_amp[64] = {0};
            uint8_t fft_out[64] = {0};
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = fft_amp[0] / (65536 / canvas_height) * vfx.scale_factor;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = fft_amp[k] / (65536 / canvas_height) * vfx.scale_factor;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                color_h = color_tmp;
                for (uint16_t i=0; i<canvas_width; i++) {
                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h, color_l);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    color_h += 8;
                    if (color_h > 511) {
                        color_h = 0;
                    }
                }

                if (++color_cnt % (128 / VFX_PERIOD) == 0) {
                    color_tmp += 8;
                    if (color_tmp > 511) {
                        color_tmp = 0;
                    }
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
        case 0x12: {   // 音樂噴泉-螺旋-線性
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_flg = 0;
            uint8_t color_cnt = 0;
            uint16_t color_h[64] = {0};
            uint16_t color_l[64] = {vfx.lightness};
            float   fft_amp[64] = {0};
            uint8_t fft_out[64] = {0};
            const uint8_t led_idx_table[][64] = {
                {
                    3, 4, 4, 3, 2, 2, 2, 3, 4, 5, 5, 5, 5, 4, 3, 2,
                    1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 5,
                    4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
                    6, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0,
                },
                {
                    3, 3, 4, 4, 4, 3, 2, 2, 2, 2, 3, 4, 5, 5, 5, 5,
                    5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6,
                    6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7,
                }
            };
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;

            gdispGFillArea(vfx_gdisp, 0, 0, canvas_width, canvas_height, 0x000000);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            for (uint16_t i=0; i<64; i++) {
                color_h[i] = i * 8;
                color_l[i] = vfx.lightness;
            }

            vfx_fft_plan = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, NULL, NULL);
            memset(vfx_fft_plan->input, 0x00, FFT_N * sizeof(float));

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);
                    break;
                }

                if (xEventGroupGetBits(user_event_group) & VFX_FFT_FULL_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_FFT_FULL_BIT);

                    xEventGroupSetBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_execute(vfx_fft_plan);

                    xEventGroupClearBits(user_event_group, VFX_FFT_EXEC_BIT);

                    fft_amp[0] = sqrt(pow(vfx_fft_plan->output[0], 2) + pow(vfx_fft_plan->output[1], 2)) / FFT_N;
                    fft_out[0] = fft_amp[0] / (65536 / canvas_height) * vfx.scale_factor;
                    if (fft_out[0] > canvas_height) {
                        fft_out[0] = canvas_height;
                    } else if (fft_out[0] < 1) {
                        fft_out[0] = 1;
                    }

                    for (uint16_t k=1; k<FFT_N/2; k++) {
                        fft_amp[k] = sqrt(pow(vfx_fft_plan->output[2*k], 2) + pow(vfx_fft_plan->output[2*k+1], 2)) / FFT_N * 2;
                        fft_out[k] = fft_amp[k] / (65536 / canvas_height) * vfx.scale_factor;
                        if (fft_out[k] > canvas_height) {
                            fft_out[k] = canvas_height;
                        } else if (fft_out[k] < 1) {
                            fft_out[k] = 1;
                        }
                    }
                }

                for (uint16_t i=0; i<canvas_width; i++) {
                    x = led_idx_table[0][i];
                    y = led_idx_table[1][i];

                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = canvas_height - fft_out[i];

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = canvas_height - fft_out[i];
                    uint8_t fill_cz = fft_out[i];

                    vfx_fill_cube(clear_x, clear_y, clear_z,
                                  clear_cx, clear_cy, clear_cz,
                                  0, 0);
                    vfx_fill_cube(fill_x, fill_y, fill_z,
                                  fill_cx, fill_cy, fill_cz,
                                  color_h[i], color_l[i]);

                    if (color_flg) {
                        if (color_h[i]-- == 0) {
                            color_h[i] = 511;
                        }
                    }
                }

                if (++color_cnt % (32 / VFX_PERIOD) == 0) {
                    color_flg = 1;
                } else {
                    color_flg = 0;
                }

                vTaskDelayUntil(&xLastWakeTime, VFX_PERIOD / portTICK_RATE_MS);
            }

            fft_destroy(vfx_fft_plan);
            vfx_fft_plan = NULL;

            break;
        }
#endif // CONFIG_SCREEN_PANEL_OUTPUT_VFX
        case VFX_MODE_PAUSE:
            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            xEventGroupWaitBits(
                user_event_group,
                VFX_RELOAD_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );

            break;
        case VFX_MODE_OFF:
        default:
            gdispGSetBacklight(vfx_gdisp, 0);

            vTaskDelay(500 / portTICK_RATE_MS);

            gdispGFillArea(vfx_gdisp, 0, 0, vfx_disp_width, vfx_disp_height, 0x000000);

            xEventGroupWaitBits(
                user_event_group,
                VFX_RELOAD_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );
loop_break:
            break;
        }
    }
}

void vfx_set_conf(struct vfx_conf *cfg)
{
    vfx.mode = cfg->mode;
    vfx.scale_factor = cfg->scale_factor;
    vfx.lightness = cfg->lightness;
    vfx.backlight = cfg->backlight;
    if (vfx_gdisp) {
        gdispGSetBacklight(vfx_gdisp, vfx.backlight);
    }
    xEventGroupSetBits(user_event_group, VFX_RELOAD_BIT | VFX_FFT_FULL_BIT);
    ESP_LOGI(TAG, "mode: 0x%02X, scale-factor: %u, lightness: 0x%04X, backlight: %u",
             vfx.mode, vfx.scale_factor, vfx.lightness, vfx.backlight);
}

struct vfx_conf *vfx_get_conf(void)
{
    return &vfx;
}

void vfx_init(void)
{
    size_t length = sizeof(struct vfx_conf);
    app_getenv("VFX_INIT_CFG", &vfx, &length);

    xEventGroupSetBits(user_event_group, VFX_FFT_FULL_BIT);

    xTaskCreatePinnedToCore(vfx_task, "vfxT", 5120, NULL, 7, NULL, 1);
}
