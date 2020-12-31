/*
 * vfx_core.c
 *
 *  Created on: 2019-07-03 20:05
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gfx.h"

#include "user/vfx.h"
#include "user/vfx_core.h"
#include "user/vfx_bitmap.h"

static float hue2rgb(float v1, float v2, float vH)
{
    if (vH < 0.0) {
        vH += 1.0;
    } else if (vH > 1.0) {
        vH -= 1.0;
    }

    if (6.0 * vH < 1.0) {
        return v1 + (v2 - v1) * 6.0 * vH;
    } else if (2.0 * vH < 1.0) {
        return v2;
    } else if (3.0 * vH < 2.0) {
        return v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0;
    } else {
        return v1;
    }
}

uint32_t hsl2rgb(float H, float S, float L)
{
    float v1, v2;
    uint8_t R, G, B;

    if (S == 0.0) {
        R = 255.0 * L;
        G = 255.0 * L;
        B = 255.0 * L;
    } else {
        if (L < 0.5) {
            v2 = L * (1.0 + S);
        } else {
            v2 = (L + S) - (L * S);
        }

        v1 = 2.0 * L - v2;

        R = 255.0 * hue2rgb(v1, v2, H + (1.0 / 3.0));
        G = 255.0 * hue2rgb(v1, v2, H);
        B = 255.0 * hue2rgb(v1, v2, H - (1.0 / 3.0));
    }

    return (uint32_t)(R << 16 | G << 8 | B);
}

#if defined(CONFIG_VFX_OUTPUT_WS2812) || defined(CONFIG_VFX_OUTPUT_CUBE0414)
inline void vfx_draw_pixel_raw(uint8_t x, uint8_t y, uint8_t z, uint32_t color)
{
#ifdef CONFIG_LED_LAYER_H
    uint8_t pixel_x = x + y * 8;
    uint8_t pixel_y = z;
#endif
#ifdef CONFIG_LED_LAYER_H_ZI
    uint8_t pixel_x = x + y * 8;
    uint8_t pixel_y = 7 - z;
#endif
#ifdef CONFIG_LED_LAYER_H_XYI
    uint8_t pixel_x = (7 - x) + (7 - y) * 8;
    uint8_t pixel_y = z;
#endif
#ifdef CONFIG_LED_LAYER_H_XYZI
    uint8_t pixel_x = (7 - x) + (7 - y) * 8;
    uint8_t pixel_y = 7 - z;
#endif
#ifdef CONFIG_LED_LAYER_V
    uint8_t pixel_x = x + z * 8;
    uint8_t pixel_y = y;
#endif
#ifdef CONFIG_LED_LAYER_V_ZI
    uint8_t pixel_x = x + z * 8;
    uint8_t pixel_y = 7 - y;
#endif
#ifdef CONFIG_LED_LAYER_V_XYI
    uint8_t pixel_x = (7 - x) + (7 - z) * 8;
    uint8_t pixel_y = y;
#endif
#ifdef CONFIG_LED_LAYER_V_XYZI
    uint8_t pixel_x = (7 - x) + (7 - z) * 8;
    uint8_t pixel_y = 7 - y;
#endif

    gdispGDrawPixel(vfx_gdisp, pixel_x, pixel_y, color);
}

inline void vfx_draw_pixel(uint8_t x, uint8_t y, uint8_t z, float color_h, float color_l)
{
    vfx_draw_pixel_raw(x, y, z, hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0));
}

void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, float color_h, float color_l)
{
    uint32_t pixel_color = hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);

    for (uint8_t i = 0; i < cx; i++) {
        for (uint8_t j = 0; j < cy; j++) {
            for (uint8_t k = 0; k < cz; k++) {
                vfx_draw_pixel_raw(x + i, y + j, z + k, pixel_color);
            }
        }
    }
}

void vfx_draw_cube_bitmap(const uint8_t *bitmap, float color_l)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    static uint16_t color_p = 0;
    static uint16_t color_h = 0;

    color_p = color_h;
    for (uint8_t i = 0; i < 64; i++) {
        uint8_t temp = *(bitmap + i);

        for (uint8_t j = 0; j < 8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_h, color_l);
            } else {
                vfx_draw_pixel(x, y, z, 0, 0);
            }

            temp <<= 1;

            if (++z == 8) {
                z = 0;

                if (++x == 8) {
                    x = 0;
                    if (++y == 8) {
                        y = 0;
                    }
                }

                if (++color_h == 512) {
                    color_h = 0;
                }
            }
        }
    }

    if ((color_h = ++color_p) == 512) {
        color_h = 0;
    }
}

void vfx_draw_layer_bitmap(uint8_t layer, const uint8_t *bitmap, float color_l)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = layer;
    static uint16_t color_p = 0;
    static uint16_t color_h = 0;

    color_p = color_h;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t temp = *(bitmap + i);

        for (uint8_t j = 0; j < 8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_h, color_l);
            } else {
                vfx_draw_pixel(x, y, z, 0, 0);
            }

            temp <<= 1;

            if (++y == 8) {
                y = 0;
                if (++x == 8) {
                    x = 0;
                }
            }

            if (++color_h == 512) {
                color_h = 0;
            }
        }
    }

    if ((color_h = ++color_p) == 512) {
        color_h = 0;
    }
}

void vfx_draw_layer_number(uint8_t num, uint8_t layer, float color_h, float color_l)
{
    uint8_t x = 0;
    uint8_t y = layer;
    uint8_t z = 0;

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t temp = vfx_bitmap_number[num][i];

        for (uint8_t j = 0; j < 8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_h, color_l);
            } else {
                vfx_draw_pixel(x, y, z, 0, 0);
            }

            temp <<= 1;

            if (++z == 8) {
                z = 0;
                if (++x == 8) {
                    x = 0;
                }
            }
        }
    }
}
#endif

void vfx_compute_freq_lin(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += sqrt(pow(data_in[0], 2) + pow(data_in[1], 2)) / FFT_N * (max_val * scale_factor / 32768.0);
    data_out[0] /= 2;
    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (uint16_t k = 1; k < FFT_N / 2; k++) {
        data_out[k] += sqrt(pow(vfx_fft_output[2 * k], 2) + pow(vfx_fft_output[2 * k + 1], 2)) / FFT_N * 2 * (max_val * scale_factor / 32768.0);
        data_out[k] /= 2;
        if (data_out[k] > max_val) {
            data_out[k] = max_val;
        } else if (data_out[k] < min_val) {
            data_out[k] = min_val;
        }
    }
}

void vfx_compute_freq_log(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += 20 * log10(1 + sqrt(pow(data_in[0], 2) + pow(data_in[1], 2)) / FFT_N) * (max_val * scale_factor / 32768.0);
    data_out[0] /= 2;
    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (uint16_t k = 1; k < FFT_N / 2; k++) {
        data_out[k] += 20 * log10(1 + sqrt(pow(data_in[2 * k], 2) + pow(data_in[2 * k + 1], 2)) / FFT_N * 2) * (max_val * scale_factor / 32768.0);
        data_out[k] /= 2;
        if (data_out[k] > max_val) {
            data_out[k] = max_val;
        } else if (data_out[k] < min_val) {
            data_out[k] = min_val;
        }
    }
}
