/*
 * vfx.c
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "esp_log.h"

#include "gfx.h"

#include "core/os.h"
#include "core/app.h"

#include "user/fft.h"
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

static coord_t vfx_disp_width = 0;
static coord_t vfx_disp_height = 0;

static GTimer vfx_flush_timer;

static void vfx_flush_task(void *pvParameter)
{
    gdispGFlush(vfx_gdisp);
}

static void vfx_task(void *pvParameter)
{
    portTickType xLastWakeTime;

    gfxInit();

    vfx_gdisp = gdispGetDisplay(0);

    gtimerStart(&vfx_flush_timer, vfx_flush_task, NULL, TRUE, TIME_INFINITE);

    ESP_LOGI(TAG, "started.");

#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
    gdispGSetOrientation(vfx_gdisp, CONFIG_LCD_ROTATION_DEGREE);
#endif

    vfx_disp_width = gdispGGetWidth(vfx_gdisp);
    vfx_disp_height = gdispGGetHeight(vfx_gdisp);

    while (1) {
        switch (vfx.mode) {
#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
        // LCD Output
        case VFX_MODE_IDX_12_BAND_R:    // 音樂頻譜-12段-彩虹
        case VFX_MODE_IDX_12_BAND_G: {  // 音樂頻譜-12段-漸變
            enum { BAND_N = 12 };
            vfx_mode_t mode = vfx.mode;
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            uint16_t backlight = vfx.backlight;
            uint16_t delay[BAND_N] = {0};
            uint16_t fft_out[BAND_N] = {0};
            float xscale[BAND_N + 1] = {0.0};
            uint16_t center_y = vfx_disp_height % 2 ? vfx_disp_height / 2 : vfx_disp_height / 2 - 1;
            const uint16_t flush_period = 20;

            fft_compute_xscale(xscale, BAND_N);

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                fft_compute_bands(fft_out, xscale, BAND_N, delay, vfx_disp_height / 2, 0);

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

                if (mode == VFX_MODE_IDX_12_BAND_R) {
                    color_h = 0;
                } else {
                    color_p = color_h;
                }

                for (uint8_t i = 0; i < BAND_N; i++) {
                    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

#if defined(CONFIG_VFX_OUTPUT_ST7735)
                    uint16_t clear_x  = i * 13;
                    uint16_t clear_cx = 11;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height;

                    uint16_t fill_x  = i * 13;
                    uint16_t fill_cx = 11;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 2;
#else
                    uint16_t clear_x  = i * 20;
                    uint16_t clear_cx = 18;
                    uint16_t clear_y  = 0;
                    uint16_t clear_cy = vfx_disp_height;

                    uint16_t fill_x  = i * 20;
                    uint16_t fill_cx = 18;
                    uint16_t fill_y  = center_y - fft_out[i];
                    uint16_t fill_cy = fft_out[i] * 2 + 1;
#endif
                    gdispGFillArea(vfx_gdisp, clear_x, clear_y, clear_cx, clear_cy, Black);
                    gdispGFillArea(vfx_gdisp, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                    if (mode == VFX_MODE_IDX_12_BAND_R) {
                        color_h += 40;
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

                if (gdispGGetBacklight(vfx_gdisp) != backlight) {
                    vTaskDelay(50 / portTICK_RATE_MS);

                    gdispGSetBacklight(vfx_gdisp, backlight);
                }

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

            break;
        }
        case VFX_MODE_IDX_SPECTRUM_R_N:     // 音樂頻譜-彩虹-線性
        case VFX_MODE_IDX_SPECTRUM_G_N: {   // 音樂頻譜-漸變-線性
            vfx_mode_t mode = vfx.mode;
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            uint16_t backlight = vfx.backlight;
            uint16_t fft_out[vfx_disp_width];
            const uint16_t flush_period = 20;

            memset(fft_out, 0x00, vfx_disp_width * sizeof(uint16_t));

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                fft_compute_lin(fft_out, vfx_disp_width, 1, vfx_disp_height, 1);

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

                if (mode == VFX_MODE_IDX_SPECTRUM_R_N) {
                    color_h = 0;
                } else {
                    color_p = color_h;
                }

                for (uint16_t i = 0; i < vfx_disp_width; i++) {
                    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

                    gdispGFillArea(vfx_gdisp, i, 0, 1, vfx_disp_height, Black);
                    gdispGFillArea(vfx_gdisp, i, vfx_disp_height - fft_out[i], 1, fft_out[i], pixel_color);

                    if (mode == VFX_MODE_IDX_SPECTRUM_R_N) {
                        if ((color_h += 2) >= 512) {
                            color_h -= 512;
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

                if (gdispGGetBacklight(vfx_gdisp) != backlight) {
                    vTaskDelay(50 / portTICK_RATE_MS);

                    gdispGSetBacklight(vfx_gdisp, backlight);
                }

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

            break;
        }
        case VFX_MODE_IDX_SPECTRUM_R_L:     // 音樂頻譜-彩虹-對數
        case VFX_MODE_IDX_SPECTRUM_G_L: {   // 音樂頻譜-漸變-對數
            vfx_mode_t mode = vfx.mode;
            uint16_t color_p = 0;
            uint16_t color_h = 0;
            uint16_t color_l = vfx.lightness;
            uint16_t backlight = vfx.backlight;
            uint16_t fft_out[vfx_disp_width];
            uint16_t center_y = vfx_disp_height % 2 ? vfx_disp_height / 2 : vfx_disp_height / 2 - 1;
            const uint16_t flush_period = 20;

            memset(fft_out, 0x00, vfx_disp_width * sizeof(uint16_t));

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                fft_compute_log(fft_out, vfx_disp_width, 1, vfx_disp_height / 2, 0);

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

                if (mode == VFX_MODE_IDX_SPECTRUM_R_L) {
                    color_h = 0;
                } else {
                    color_p = color_h;
                }

                for (uint16_t i = 0; i < vfx_disp_width; i++) {
                    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

                    gdispGFillArea(vfx_gdisp, i, 0, 1, vfx_disp_height, Black);
                    gdispGFillArea(vfx_gdisp, i, center_y - fft_out[i], 1, fft_out[i] * 2 + 1, pixel_color);

                    if (mode == VFX_MODE_IDX_SPECTRUM_R_L) {
                        if ((color_h += 2) >= 512) {
                            color_h -= 512;
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

                if (gdispGGetBacklight(vfx_gdisp) != backlight) {
                    vTaskDelay(50 / portTICK_RATE_MS);

                    gdispGSetBacklight(vfx_gdisp, backlight);
                }

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

            break;
        }
        case VFX_MODE_IDX_SPECTRUM_M_N:     // 音樂頻譜-電平-線性
        case VFX_MODE_IDX_SPECTRUM_M_L: {   // 音樂頻譜-電平-對數
            vfx_mode_t mode = vfx.mode;
            uint16_t color_h = 0;
            uint16_t color_p = 432;
            uint16_t color_l = vfx.lightness;
            uint16_t backlight = vfx.backlight;
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
            uint16_t fft_out[vu_idx_max + 1];
            uint8_t vu_peak_value[vu_idx_max + 1];
            uint8_t vu_peak_delay[vu_idx_max + 1];
            uint8_t vu_drop_delay[vu_idx_max + 1];
            const uint16_t vu_peak_delay_max = 19;
            const uint16_t vu_drop_delay_max = 1;
            const uint16_t flush_period = 20;

            memset(fft_out, 0x00, (vu_idx_max + 1) * sizeof(uint16_t));

            memset(vu_peak_value, 0x00, (vu_idx_max + 1) * sizeof(uint8_t));
            memset(vu_peak_delay, vu_peak_delay_max, (vu_idx_max + 1) * sizeof(uint8_t));
            memset(vu_drop_delay, vu_drop_delay_max, (vu_idx_max + 1) * sizeof(uint8_t));

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                if (mode == VFX_MODE_IDX_SPECTRUM_M_N) {
                    fft_compute_lin(fft_out, vu_idx_max + 1, FFT_N / 64, vu_val_max, 0);
                } else {
                    fft_compute_log(fft_out, vu_idx_max + 1, FFT_N / 64, vu_val_max, 0);
                }

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

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
                            vu_drop_delay[i] = vu_drop_delay_max;
                            vu_peak_value[i]--;
                        }
                    }
                    if (vu_peak_value[i] <= vu_val_out) {
                        vu_peak_value[i] = vu_val_out;
                        vu_peak_delay[i] = vu_peak_delay_max;
                    }
                    if (vu_peak_value[i] != vu_val_out) {
                        gdispGFillArea(vfx_gdisp, i * vu_width + 1, (vu_val_max - vu_peak_value[i]) * vu_height + 1, vu_width - 2, vu_height - 2, Black);
                    }

                    uint32_t peak_color = hsl2rgb(color_p / 511.0, 1.0, color_l / 511.0);
                    gdispGFillArea(vfx_gdisp, i * vu_width + 1, (vu_val_max - vu_peak_value[i]) * vu_height + 1, vu_width - 2, vu_height - 2, peak_color);

                    color_h = 0;
                    for (int8_t j = vu_val_max; j >= vu_val_min; j--) {
                        uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

                        if (j == vu_peak_value[i]) {
                            continue;
                        }

                        if (j > vu_val_out || (j == 0 && vu_val_out == 0)) {
                            gdispGFillArea(vfx_gdisp, i * vu_width + 1, (vu_val_max - j) * vu_height + 1, vu_width - 2, vu_height - 2, Black);
                        } else {
                            gdispGFillArea(vfx_gdisp, i * vu_width + 1, (vu_val_max - j) * vu_height + 1, vu_width - 2, vu_height - 2, pixel_color);
                        }

                        color_h += vu_step;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                if (gdispGGetBacklight(vfx_gdisp) != backlight) {
                    vTaskDelay(50 / portTICK_RATE_MS);

                    gdispGSetBacklight(vfx_gdisp, backlight);
                }

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

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
            const uint16_t flush_period = 20;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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
            const uint16_t flush_period = 20;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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
            const uint16_t flush_period = 20;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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
            const uint16_t flush_period = 10;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                vfx_fill_cube(0, 0, 0, 8, 8, 8, color_h, fade_cnt * color_l / 255.0);

                if (scale_dir == 0) {
                    if (++fade_cnt == 256) {
                        fade_cnt = 255;

                        scale_dir = 1;
                    }
                } else {
                    if (fade_cnt-- == 0) {
                        fade_cnt = 0;

                        color_h = esp_random() % 512;

                        scale_dir = 0;
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
            int16_t led_base = 0;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_h[512] = {0};
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 10;

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
                uint16_t r = esp_random() % 512;
                uint16_t t = led_idx[r];
                led_idx[r] = led_idx[i];
                led_idx[i] = t;
            }

            for (uint16_t i = 0; i < 512; i++) {
                uint16_t r = esp_random() % 512;
                uint16_t t = color_h[r];
                color_h[r] = color_h[i];
                color_h[i] = t;
            }

            led_base = -led_num;
            while (1) {
                for (uint16_t i = 0; i < 256; i++) {
                    xLastWakeTime = xTaskGetTickCount();

                    if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                        goto exit;
                    }

                    if (led_base >= 0) {
                        x = (led_idx[led_base] % 64) % 8;
                        y = (led_idx[led_base] % 64) / 8;
                        z = led_idx[led_base] / 64;

                        vfx_draw_pixel(x, y, z, color_h[led_base], (255 - i) * color_l / 255.0);
                    }

                    if ((led_base + led_num) < 512) {
                        x = (led_idx[led_base + led_num] % 64) % 8;
                        y = (led_idx[led_base + led_num] % 64) / 8;
                        z = led_idx[led_base + led_num] / 64;

                        vfx_draw_pixel(x, y, z, color_h[led_base + led_num], i * color_l / 255.0);
                    }

                    gtimerJab(&vfx_flush_timer);

                    vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
                }

                if (++led_base == 512) {
                    led_base = -led_num;
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

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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

            number  = esp_random() % 10;
            color_h = esp_random() % 512;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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
            const uint16_t flush_period = 20;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
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
        case VFX_MODE_IDX_ROTATING_F:     // 旋轉曲面-正
        case VFX_MODE_IDX_ROTATING_B: {   // 旋轉曲面-反
            vfx_mode_t mode = vfx.mode;
            uint16_t frame_p = 0;
            uint16_t frame_i = 0;
            uint16_t color_l = vfx.lightness;
            const uint16_t flush_period = 40;

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                frame_p = frame_i;
                for (uint8_t i = 0; i < 8; i++) {
                    vfx_draw_layer_bitmap(i, vfx_bitmap_line[frame_i], color_l);

                    if (mode == VFX_MODE_IDX_ROTATING_F && frame_i-- == 0) {
                        frame_i = 27;
                    } else if (mode == VFX_MODE_IDX_ROTATING_B && ++frame_i == 28) {
                        frame_i = 0;
                    }
                }

                if (mode == VFX_MODE_IDX_ROTATING_F && frame_p-- == 0) {
                    frame_i = 27;
                } else if (mode == VFX_MODE_IDX_ROTATING_B && ++frame_p == 28) {
                    frame_i = 0;
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
            vfx_mode_t mode = vfx.mode;
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t  color_d = 0;
            uint16_t color_p = 504;
            uint16_t color_h = 504;
            uint16_t color_l = vfx.lightness;
            uint16_t fft_out[64] = {0};
            const uint16_t flush_period = 20;

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                if (mode == VFX_MODE_IDX_FOUNTAIN_S_N || mode == VFX_MODE_IDX_FOUNTAIN_G_N) {
                    fft_compute_lin(fft_out, 64, FFT_N / 64, 8, 1);
                } else {
                    fft_compute_log(fft_out, 64, FFT_N / 64, 8, 1);
                }

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

                if (mode == VFX_MODE_IDX_FOUNTAIN_S_N || mode == VFX_MODE_IDX_FOUNTAIN_S_L) {
                    color_h = 504;
                } else {
                    color_h = color_p;
                }

                for (uint8_t i = 0; i < 64; i++) {
                    vfx_fill_cube(x, 7 - y, 0, 1, 1, 8 - fft_out[i], 0, 0);
                    vfx_fill_cube(x, 7 - y, 8 - fft_out[i], 1, 1, fft_out[i], color_h, color_l);

                    if (++y == 8) {
                        y = 0;
                        if (++x == 8) {
                            x = 0;
                        }
                    }

                    if ((color_h - 8) < 0) {
                        color_h += 504;
                    } else {
                        color_h -= 8;
                    }
                }

                if (++color_d == 2) {
                    color_d = 0;
                    if (color_p-- == 0) {
                        color_p = 511;
                    }
                }

                gtimerJab(&vfx_flush_timer);

                vTaskDelayUntil(&xLastWakeTime, flush_period / portTICK_RATE_MS);
            }

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

            break;
        }
        case VFX_MODE_IDX_FOUNTAIN_H_N:     // 音樂噴泉-螺旋-線性
        case VFX_MODE_IDX_FOUNTAIN_H_L: {   // 音樂噴泉-螺旋-對數
            vfx_mode_t mode = vfx.mode;
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_n = 0;
            uint8_t color_d = 0;
            uint16_t color_h[64] = {0};
            uint16_t color_l = vfx.lightness;
            uint16_t fft_out[64] = {0};
            const uint8_t led_idx_table[2][64] = {
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
            const uint16_t flush_period = 20;

            for (uint8_t i = 0; i < 64; i++) {
                color_h[i] = i * 8;
            }

            xEventGroupSetBits(user_event_group, VFX_FFT_MODE_BIT);

            fft_init();

            while (1) {
                xLastWakeTime = xTaskGetTickCount();

                if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
                    break;
                }

                if (!(xEventGroupGetBits(user_event_group) & VFX_FFT_IDLE_BIT)) {
                    fft_execute(vfx.scale_factor / 1000.0);
                }

                if (mode == VFX_MODE_IDX_FOUNTAIN_H_N) {
                    fft_compute_lin(fft_out, 64, FFT_N / 64, 8, 1);
                } else {
                    fft_compute_log(fft_out, 64, FFT_N / 64, 8, 1);
                }

                xEventGroupSetBits(user_event_group, VFX_FFT_IDLE_BIT);

                for (uint8_t i = 0; i < 64; i++) {
                    x = led_idx_table[0][i];
                    y = led_idx_table[1][i];

                    vfx_fill_cube(x, 7 - y, 0, 1, 1, 8 - fft_out[i], 0, 0);
                    vfx_fill_cube(x, 7 - y, 8 - fft_out[i], 1, 1, fft_out[i], color_h[i], color_l);

                    if (color_n && color_h[i]-- == 0) {
                        color_h[i] = 511;
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

            xEventGroupClearBits(user_event_group, VFX_FFT_MODE_BIT);

            break;
        }
#endif
        case VFX_MODE_IDX_PAUSE:
#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
            gdispGSetBacklight(vfx_gdisp, vfx.backlight);
#endif
            /* fall through */
        case VFX_MODE_IDX_OFF:
        default:
            xEventGroupWaitBits(
                user_event_group,
                VFX_RLD_MODE_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY
            );

            break;
        }

        if (xEventGroupGetBits(user_event_group) & VFX_RLD_MODE_BIT) {
            xEventGroupClearBits(user_event_group, VFX_RLD_MODE_BIT);

#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
            gdispGSetBacklight(vfx_gdisp, 0);

            vTaskDelay(500 / portTICK_RATE_MS);
#endif

            if (vfx.mode != VFX_MODE_IDX_PAUSE) {
                gdispGClear(vfx_gdisp, Black);

                gtimerJab(&vfx_flush_timer);
            }
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

    ESP_LOGI(TAG, "mode: 0x%02X, scale-factor: 0x%04X, lightness: 0x%04X, backlight: %u",
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
