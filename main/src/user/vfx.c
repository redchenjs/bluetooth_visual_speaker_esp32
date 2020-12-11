/*
 * vfx.c
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "fft.h"
#include "gfx.h"

#include "core/os.h"
#include "core/app.h"
#include "user/vfx.h"
#include "user/vfx_core.h"
#include "user/vfx_bitmap.h"

#define TAG "vfx"

static vfx_config_t vfx = {
    .mode = DEFAULT_VFX_MODE,
    .scale_factor = DEFAULT_VFX_SCALE_FACTOR,
    .lightness = DEFAULT_VFX_LIGHTNESS,
    .backlight = DEFAULT_VFX_BACKLIGHT
};

GDisplay *vfx_gdisp = NULL;

float vfx_fft_input[FFT_N] = {0.0};
float vfx_fft_output[FFT_N] = {0.0};

static coord_t vfx_disp_width = 0;
static coord_t vfx_disp_height = 0;

static GTimer vfx_flush_timer;

#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
static const char *img_file_ptr[][2] = {
#ifdef CONFIG_VFX_OUTPUT_ST7735
    [VFX_MODE_IDX_GIF_NYAN_CAT] = {ani0_160x80_gif_ptr, ani0_160x80_gif_end},
    [VFX_MODE_IDX_GIF_BILIBILI] = {ani1_160x80_gif_ptr, ani1_160x80_gif_end}
#else
    [VFX_MODE_IDX_GIF_NYAN_CAT] = {ani0_240x135_gif_ptr, ani0_240x135_gif_end},
    [VFX_MODE_IDX_GIF_BILIBILI] = {ani1_240x135_gif_ptr, ani1_240x135_gif_end}
#endif
};
#endif

static void vfx_flush_task(void *pvParameter)
{
    gdispGFlush(vfx_gdisp);
}

static void vfx_task(void *pvParameter)
{
    portTickType xLastWakeTime;

    gfxInit();

    vfx_gdisp = gdispGetDisplay(0);
    vfx_disp_width = gdispGGetWidth(vfx_gdisp);
    vfx_disp_height = gdispGGetHeight(vfx_gdisp);

    gtimerStart(&vfx_flush_timer, vfx_flush_task, NULL, TRUE, TIME_INFINITE);

    ESP_LOGI(TAG, "started.");

#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
    gdispGSetOrientation(vfx_gdisp, CONFIG_LCD_ROTATION_DEGREE);
#endif

    while (1) {
        switch (vfx.mode) {
#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
        // LCD Output
        case VFX_MODE_IDX_GIF_NYAN_CAT:
        case VFX_MODE_IDX_GIF_BILIBILI: {   // 動態貼圖
            gdispImage gfx_image;

            if (!(gdispImageOpenMemory(&gfx_image, img_file_ptr[vfx.mode][0]) & GDISP_IMAGE_ERR_UNRECOVERABLE)) {
                gdispImageSetBgColor(&gfx_image, Black);

                gdispGSetBacklight(vfx_gdisp, vfx.backlight);

                while (1) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                        break;
                    }

                    if (gdispImageDraw(&gfx_image, 0, 0, gfx_image.width, gfx_image.height, 0, 0) != GDISP_IMAGE_ERR_OK) {
                        ESP_LOGE(TAG, "failed to draw image: %u", vfx.mode);
                        vfx.mode = VFX_MODE_IDX_OFF;
                        break;
                    }

                    gtimerJab(&vfx_flush_timer);

                    delaytime_t delay = gdispImageNext(&gfx_image);
                    if (delay == TIME_INFINITE) {
                        vfx.mode = VFX_MODE_IDX_PAUSE;
                        break;
                    }

                    if (delay != TIME_IMMEDIATE) {
                        vTaskDelayUntil(&xLastWakeTime, delay / portTICK_RATE_MS);
                    }
                }

                gdispImageClose(&gfx_image);
            } else {
                ESP_LOGE(TAG, "failed to open image: %u", vfx.mode);
                vfx.mode = VFX_MODE_IDX_OFF;
                break;
            }
            break;
        }
        case VFX_MODE_IDX_SPECTRUM_R_N:     // 音樂頻譜-彩虹-線性
        case VFX_MODE_IDX_SPECTRUM_G_N: {   // 音樂頻譜-漸變-線性
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            fft_config_t *fft = NULL;
            uint16_t fft_out[64] = {0};
            const uint16_t flush_period = 20;

            xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
            fft = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, vfx_fft_input, vfx_fft_output);

            xEventGroupSetBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(fft);
                    xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);
                }

                vfx_compute_freq_lin(vfx_fft_output, fft_out, vfx.scale_factor * 4, vfx_disp_height, 1);

                if (vfx.mode == VFX_MODE_IDX_SPECTRUM_R_N) {
                    color_h = 0;
                } else {
                    color_p = color_h;
                }

                for (uint16_t i = 0; i < vfx_disp_width; i++) {
                    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

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

                    gdispGFillArea(vfx_gdisp, clear_x, clear_y, clear_cx, clear_cy, Black);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if (vfx.mode == VFX_MODE_IDX_SPECTRUM_R_N) {
                        if ((color_h += 8) == 512) {
                            color_h = 0;
                        }
                    } else {
                        if (++color_h == 512) {
                            color_h = 0;
                        }
                    }
                }

                if (++color_p == 512) {
                    color_h = 0;
                } else {
                    color_h = color_p;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            fft_destroy(fft);

            break;
        }
        case VFX_MODE_IDX_SPECTRUM_R_L:     // 音樂頻譜-彩虹-對數
        case VFX_MODE_IDX_SPECTRUM_G_L: {   // 音樂頻譜-漸變-對數
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            fft_config_t *fft = NULL;
            uint16_t fft_out[64] = {0};
            uint16_t center_y = vfx_disp_height % 2 ? vfx_disp_height / 2 : vfx_disp_height / 2 - 1;
            const uint16_t flush_period = 20;

            xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
            fft = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, vfx_fft_input, vfx_fft_output);

            xEventGroupSetBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(fft);
                    xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);
                }

                vfx_compute_freq_log(vfx_fft_output, fft_out, vfx.scale_factor * 2, vfx_disp_height, 0);

                if (vfx.mode == VFX_MODE_IDX_SPECTRUM_R_L) {
                    color_h = 0;
                } else {
                    color_p = color_h;
                }

                for (uint16_t i = 0; i < vfx_disp_width; i++) {
                    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

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

                    gdispGFillArea(vfx_gdisp, clear_x, clear_u_y, clear_cx, clear_cy, Black);
                    gdispGFillArea(vfx_gdisp, clear_x, clear_d_y, clear_cx, clear_cy, Black);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if (vfx.mode == VFX_MODE_IDX_SPECTRUM_R_L) {
                        if ((color_h += 8) == 512) {
                            color_h = 0;
                        }
                    } else {
                        if (++color_h == 512) {
                            color_h = 0;
                        }
                    }
                }

                if (++color_p == 512) {
                    color_h = 0;
                } else {
                    color_h = color_p;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            fft_destroy(fft);

            break;
        }
        case VFX_MODE_IDX_SPECTRUM_M_N:     // 音樂頻譜-格柵-線性
        case VFX_MODE_IDX_SPECTRUM_M_L: {   // 音樂頻譜-格柵-對數
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            fft_config_t *fft = NULL;
            uint16_t fft_out[64] = {0};
#if defined(CONFIG_VFX_OUTPUT_ST7735)
            const uint8_t vu_idx_min = 0;
            const uint8_t vu_idx_max = 19;
            const uint8_t vu_val_min = 0;
            const uint8_t vu_val_max = 19;
            const uint8_t vu_height = 4;
            const uint8_t vu_width = 8;
            const uint8_t vu_step = 9;
#else
            const uint8_t vu_idx_min = 0;
            const uint8_t vu_idx_max = 23;
            const uint8_t vu_val_min = 0;
            const uint8_t vu_val_max = 26;
            const uint8_t vu_height = 5;
            const uint8_t vu_width = 10;
            const uint8_t vu_step = 6;
#endif
            static uint8_t vu_peak_value[24] = {0};
            static uint8_t vu_peak_delay[24] = {0};
            static uint8_t vu_drop_delay[24] = {0};
            const uint8_t vu_peak_delay_init = 18;
            const uint8_t vu_drop_delay_init = 2;
            const uint16_t flush_period = 20;

            xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            gdispGSetBacklight(vfx_gdisp, vfx.backlight);

            memset(vu_peak_delay, vu_peak_delay_init - 1, sizeof(vu_peak_delay));
            memset(vu_drop_delay, vu_drop_delay_init - 1, sizeof(vu_drop_delay));

            memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
            fft = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, vfx_fft_input, vfx_fft_output);

            xEventGroupSetBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(fft);
                    xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);
                }

                if (vfx.mode == VFX_MODE_IDX_SPECTRUM_M_N) {
                    vfx_compute_freq_lin(vfx_fft_output, fft_out, vfx.scale_factor * 4, vu_val_max, 0);
                } else {
                    vfx_compute_freq_log(vfx_fft_output, fft_out, vfx.scale_factor * 4, vu_val_max, 0);
                }

                for (uint8_t i = vu_idx_min; i <= vu_idx_max; i++) {
                    int16_t vu_val_out = fft_out[i];

                    if (vu_val_out > vu_val_max) {
                        vu_val_out = vu_val_max;
                    } else if (vu_val_out < vu_val_min) {
                        vu_val_out = vu_val_min;
                    }

                    if (vu_peak_delay[i]-- == 0) {
                        vu_peak_delay[i] = 0;
                        if (vu_drop_delay[i]-- == 0) {
                            vu_drop_delay[i] = vu_drop_delay_init - 1;
                            vu_peak_value[i]--;
                        }
                    }
                    if (vu_peak_value[i] <= vu_val_out) {
                        vu_peak_value[i] = vu_val_out;
                        vu_peak_delay[i] = vu_peak_delay_init - 1 + vu_peak_delay[i] % vu_drop_delay_init;
                    }
                    if (vu_peak_value[i] != vu_val_out) {
                        gdispGFillArea(vfx_gdisp, i*vu_width+1, (vu_val_max-vu_peak_value[i])*vu_height+1, vu_width-2, vu_height-2, Black);
                    }

                    uint32_t peak_color = hsl2rgb(432 / 511.0, 1.0, color_l / 511.0);
                    gdispGFillArea(vfx_gdisp, i*vu_width+1, (vu_val_max-vu_peak_value[i])*vu_height+1, vu_width-2, vu_height-2, peak_color);

                    color_h = 0;
                    for (int8_t j = vu_val_max; j >= vu_val_min; j--) {
                        uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

                        if (j == vu_peak_value[i]) {
                            continue;
                        }

                        if (j > vu_val_out || ((j == 0) && (vu_val_out == 0))) {
                            // upside
                            gdispGFillArea(vfx_gdisp, i*vu_width+1, (vu_val_max-j)*vu_height+1, vu_width-2, vu_height-2, Black);
                        } else {
                            // underside
                            gdispGFillArea(vfx_gdisp, i*vu_width+1, (vu_val_max-j)*vu_height+1, vu_width-2, vu_height-2, pixel_color);
                        }

                        color_h += vu_step;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            fft_destroy(fft);

            break;
        }
#else
        // Light Cube Output
        case VFX_MODE_IDX_RANDOM: {   // 隨機
            vfx.mode = esp_random() % VFX_MODE_IDX_MAX;

            break;
        }
        case VFX_MODE_IDX_RAINBOW: {   // 彩虹
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 16;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                while (1) {
                    vfx_draw_pixel(x, y, z, color_h, color_l);

                    if (++x == 8) {
                        x = 0;
                        if (++y == 8) {
                            y = 0;
                            if (++z == 8) {
                                z = 0;
                                break;
                            }
                        }
                    }

                    if (++color_h == 512) {
                        color_h = 0;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_RIBBON: {   // 彩帶
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 16;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                color_p = color_h;
                while (1) {
                    for (uint8_t i = 0; i < 8; i++) {
                        vfx_draw_pixel(x, y, i, color_h, color_l);
                    }

                    if (++x == 8) {
                        x = 0;
                        if (++y == 8) {
                            y = 0;
                            break;
                        }
                    }

                    if (++color_h == 512) {
                        color_h = 0;
                    }
                }

                if (++color_p == 512) {
                    color_h = 0;
                } else {
                    color_h = color_p;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_GRADUAL: {   // 漸變
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 16;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                vfx_fill_cube(0, 0, 0, 8, 8, 8, color_h, color_l);

                if (++color_h == 512) {
                    color_h = 0;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_BREATHING: {   // 呼吸
            uint8_t scale_dir = 0;
            uint16_t fade_cnt = 0;
            uint16_t color_h = esp_random() % 512;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 8;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                vfx_fill_cube(0, 0, 0, 8, 8, 8, color_h, fade_cnt * color_l / 256.0);

                if (scale_dir == 0) {
                    // 暗->明
                    if (++fade_cnt == 256) {
                        scale_dir = 1;
                    }
                } else {
                    // 明->暗
                    if (--fade_cnt == 0) {
                        scale_dir = 0;
                        color_h = esp_random() % 512;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_STARSKY_R:     // 星空-紫紅
        case VFX_MODE_IDX_STARSKY_G:     // 星空-黃綠
        case VFX_MODE_IDX_STARSKY_B: {   // 星空-靑藍
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_h[512] = {0};
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 8;

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            for (uint16_t i = 0; i < 512; i++) {
                led_idx[i] = i;
                switch (vfx.mode) {
                    case VFX_MODE_IDX_STARSKY_R: color_h[i] = i % 80 + 432; break;
                    case VFX_MODE_IDX_STARSKY_G: color_h[i] = i % 85 + 80;  break;
                    case VFX_MODE_IDX_STARSKY_B: color_h[i] = i % 80 + 260; break;
                    default: break;
                }
            }

            for (uint16_t i = 0; i < 512; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = led_idx[rnd];
                led_idx[rnd] = led_idx[i];
                led_idx[i] = tmp;
            }

            for (uint16_t i = 0; i < 512; i++) {
                uint16_t rnd = esp_random() % 512;
                uint16_t tmp = color_h[rnd];
                color_h[rnd] = color_h[i];
                color_h[i] = tmp;
            }

            int16_t idx_base = -led_num;
            while (1) {
                for (uint16_t i = 0; i <= 256; i++) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                        xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                        goto exit;
                    }

                    if (idx_base >= 0) {
                        x = (led_idx[idx_base] % 64) % 8;
                        y = (led_idx[idx_base] % 64) / 8;
                        z = led_idx[idx_base] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base], (256 - i) * color_l / 256.0);
                    }

                    if ((idx_base + led_num) < 512) {
                        x = (led_idx[idx_base + led_num] % 64) % 8;
                        y = (led_idx[idx_base + led_num] % 64) / 8;
                        z = led_idx[idx_base + led_num] / 64;
                        vfx_draw_pixel(x, y, z, color_h[idx_base + led_num], i * color_l / 256.0);
                    }

                    gtimerJab(&vfx_flush_timer);

                    vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
                }

                if (++idx_base == 512) {
                    idx_base = -led_num;
                }
            }
exit:
            break;
        }
        case VFX_MODE_IDX_NUMBERS_S: {   // 數字-固定
            uint16_t number = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 1000;

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                number  = esp_random() % 10;
                color_h = esp_random() % 512;

                vfx_draw_layer_number(number, 2, color_h, color_l);
                vfx_draw_layer_number(number, 3, color_h, color_l);
                vfx_draw_layer_number(number, 4, color_h, color_l);
                vfx_draw_layer_number(number, 5, color_h, color_l);

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_NUMBERS_D: {   // 數字-滾動
            uint16_t number = 0;
            uint16_t layer0 = 0;
            uint16_t layer1 = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 80;

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            number  = esp_random() % 10;
            color_h = esp_random() % 512;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                for (uint8_t i = layer0; i <= layer1; i++) {
                    vfx_draw_layer_number(number, i, color_h, color_l);
                }

                if (layer1 != 7 && layer0 != 0) {
                    vfx_draw_layer_number(number, layer0, 0, 0);

                    layer1++;
                    layer0++;
                } else if (layer1 == 7) {
                    if (++layer0 == 8) {
                        vfx_draw_layer_number(number, 7, 0, 0);

                        layer0 = 0;
                        layer1 = 0;

                        number  = esp_random() % 10;
                        color_h = esp_random() % 512;
                    } else {
                        vfx_draw_layer_number(number, layer0 - 1, 0, 0);
                    }
                } else {
                    if ((layer1 - layer0) != 4) {
                        layer1++;
                    } else {
                        vfx_draw_layer_number(number, 0, 0, 0);

                        layer1++;
                        layer0++;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_MAGIC_CARPET: {   // 魔毯
            uint16_t frame_i = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 16;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                vfx_draw_cube_bitmap(vfx_bitmap_wave[frame_i], color_l);

                if (++frame_i == 45) {
                    frame_i = 8;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_ROTATING_F: {   // 旋轉曲面-正
            uint16_t frame_p = 0;
            uint16_t frame_i = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 40;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                frame_p = frame_i;
                for (uint8_t i = 0; i < 8; i++) {
                    vfx_draw_layer_bitmap(i, vfx_bitmap_line[frame_i], color_l);

                    if (++frame_i == 28) {
                        frame_i = 0;
                    }
                }

                if (++frame_p == 28) {
                    frame_i = 0;
                } else {
                    frame_i = frame_p;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_ROTATING_B: {   // 旋轉曲面-反
            uint16_t frame_p = 0;
            uint16_t frame_i = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 40;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                frame_p = frame_i;
                for (uint8_t i = 0; i < 8; i++) {
                    vfx_draw_layer_bitmap(i, vfx_bitmap_line[frame_i], color_l);

                    if (frame_i-- == 0) {
                        frame_i = 27;
                    }
                }

                if (frame_p-- == 0) {
                    frame_i = 27;
                } else {
                    frame_i = frame_p;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }
            break;
        }
        case VFX_MODE_IDX_FOUNTAIN_S_N:     // 音樂噴泉-靜態-線性
        case VFX_MODE_IDX_FOUNTAIN_S_L:     // 音樂噴泉-靜態-對數
        case VFX_MODE_IDX_FOUNTAIN_G_N:     // 音樂噴泉-漸變-線性
        case VFX_MODE_IDX_FOUNTAIN_G_L: {   // 音樂噴泉-漸變-對數
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t  color_d = 0;
            uint16_t color_p = 504;
            uint16_t color_h = 504;
            uint16_t color_l = vfx.lightness;
            fft_config_t *fft = NULL;
            uint16_t fft_out[64] = {0};
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;
            const uint16_t flush_period = 16;

            xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);

            memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
            fft = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, vfx_fft_input, vfx_fft_output);

            xEventGroupSetBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(fft);
                    xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);
                }

                if (vfx.mode == VFX_MODE_IDX_FOUNTAIN_S_N || vfx.mode == VFX_MODE_IDX_FOUNTAIN_G_N) {
                    vfx_compute_freq_lin(vfx_fft_output, fft_out, vfx.scale_factor * 4, canvas_height, 1);
                } else {
                    vfx_compute_freq_log(vfx_fft_output, fft_out, vfx.scale_factor * 4, canvas_height, 1);
                }

                if (vfx.mode == VFX_MODE_IDX_FOUNTAIN_S_N || vfx.mode == VFX_MODE_IDX_FOUNTAIN_S_L) {
                    color_h = 504;
                } else {
                    color_h = color_p;
                }

                for (uint16_t i = 0; i < canvas_width; i++) {
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

                    if (++y == 8) {
                        y = 0;
                        if (++x == 8) {
                            x = 0;
                        }
                    }

                    if (color_h >= 8) {
                        color_h -= 8;
                    } else {
                        color_h += 504;
                    }
                }

                if (++color_d == 2) {
                    color_d = 0;
                    if (++color_p == 512) {
                        color_p = 0;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            fft_destroy(fft);

            break;
        }
        case VFX_MODE_IDX_FOUNTAIN_H_N:     // 音樂噴泉-螺旋-線性
        case VFX_MODE_IDX_FOUNTAIN_H_L: {   // 音樂噴泉-螺旋-對數
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_n = 0;
            uint8_t color_d = 0;
            uint16_t color_h[64] = {0};
            uint16_t color_l = vfx.lightness;
            fft_config_t *fft = NULL;
            uint16_t fft_out[64] = {0};
            const uint8_t led_idx_table[][64] = {
                {
                    3, 4, 4, 3, 2, 2, 2, 3, 4, 5, 5, 5, 5, 4, 3, 2,
                    1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 6, 5,
                    4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
                    6, 7, 7, 7, 7, 7, 7, 7, 7, 6, 5, 4, 3, 2, 1, 0
                }, {
                    3, 3, 4, 4, 4, 3, 2, 2, 2, 2, 3, 4, 5, 5, 5, 5,
                    5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6,
                    6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7, 7
                }
            };
            const coord_t canvas_width = 64;
            const coord_t canvas_height = 8;
            const uint16_t flush_period = 16;

            xEventGroupClearBits(user_event_group, VFX_FFT_IDLE_BIT);

            for (uint16_t i = 0; i < 64; i++) {
                color_h[i] = 504 - i * 8;
            }

            memset(vfx_fft_input, 0x00, sizeof(vfx_fft_input));
            fft = fft_init(FFT_N, FFT_REAL, FFT_FORWARD, vfx_fft_input, vfx_fft_output);

            xEventGroupSetBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(fft);
                    xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);
                }

                if (vfx.mode == VFX_MODE_IDX_FOUNTAIN_H_N) {
                    vfx_compute_freq_lin(vfx_fft_output, fft_out, vfx.scale_factor * 4, canvas_height, 1);
                } else {
                    vfx_compute_freq_log(vfx_fft_output, fft_out, vfx.scale_factor * 4, canvas_height, 1);
                }

                for (uint16_t i = 0; i < canvas_width; i++) {
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
                                  color_h[i], color_l);

                    if (color_n) {
                        if (++color_h[i] == 512) {
                            color_h[i] = 0;
                        }
                    }
                }

                if (++color_d == 2) {
                    color_d = 0;
                    color_n = 1;
                } else {
                    color_n = 0;
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, AUDIO_INPUT_FFT_BIT);

            fft_destroy(fft);

            break;
        }
#endif
        case VFX_MODE_IDX_PAUSE:
            xEventGroupWaitBits(
                user_event_group,
                VFX_RLD_MODE_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );

            break;
        case VFX_MODE_IDX_OFF:
        default:
            gdispGSetBacklight(vfx_gdisp, 0);

            vTaskDelay(500 / portTICK_RATE_MS);

            gdispGClear(vfx_gdisp, Black);
            gtimerJab(&vfx_flush_timer);

            xEventGroupWaitBits(
                user_event_group,
                VFX_RLD_MODE_BIT,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );

            break;
        }
    }
}

void vfx_set_conf(vfx_config_t *cfg)
{
    vfx.mode = cfg->mode;
    vfx.scale_factor = cfg->scale_factor;
    vfx.lightness = cfg->lightness;
    vfx.backlight = cfg->backlight;

    xEventGroupSetBits(user_event_group, VFX_RLD_MODE_BIT);

    ESP_LOGI(TAG, "mode: 0x%02X, scale-factor: %u, lightness: 0x%04X, backlight: %u",
             vfx.mode, vfx.scale_factor, vfx.lightness, vfx.backlight);
}

vfx_config_t *vfx_get_conf(void)
{
    return &vfx;
}

void vfx_init(void)
{
    size_t length = sizeof(vfx_config_t);
    app_getenv("VFX_INIT_CFG", &vfx, &length);

    vfx_set_conf(&vfx);

    xTaskCreatePinnedToCore(vfx_task, "vfxT", 5120, NULL, 7, NULL, 1);
}
