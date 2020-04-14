/*
 * vfx_core.c
 *
 *  Created on: 2019-07-03 20:05
 *      Author: Jack Chen <redchenjs@live.com>
 */

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

static uint32_t hsl2rgb(float H, float S, float L)
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

uint32_t vfx_get_color(float color_h, float color_l)
{
    return hsl2rgb(color_h / 511.0, 1.0, color_l / 511.0);
}

#ifndef CONFIG_SCREEN_PANEL_OUTPUT_VFX
void vfx_draw_pixel(uint8_t x, uint8_t y, uint8_t z, float color_h, float color_l)
{
    uint32_t pixel_color = vfx_get_color(color_h, color_l);

#ifdef CONFIG_VFX_OUTPUT_CUBE0414
    #ifdef CONFIG_CUBE0414_LAYER_H
        uint8_t pixel_x = x + y * 8;
        uint8_t pixel_y = z;
    #endif
    #ifdef CONFIG_CUBE0414_LAYER_H_XYI
        uint8_t pixel_x = (7 - x) + (7 - y) * 8;
        uint8_t pixel_y = z;
    #endif
    #ifdef CONFIG_CUBE0414_LAYER_V
        uint8_t pixel_x = x + z * 8;
        uint8_t pixel_y = y;
    #endif
    #ifdef CONFIG_CUBE0414_LAYER_V_XYI
        uint8_t pixel_x = (7 - x) + (7 - z) * 8;
        uint8_t pixel_y = y;
    #endif
#else
    uint8_t pixel_x = x + y * 8;
    uint8_t pixel_y = z;
#endif

#ifdef CONFIG_VFX_OUTPUT_CUBE0414
    gdispGDrawPixel(vfx_gdisp, pixel_x, pixel_y, pixel_color);
#elif defined(CONFIG_VFX_OUTPUT_ST7735)
    if (pixel_x <= 31) {
        gdispGFillArea(vfx_gdisp, pixel_x * 5, pixel_y * 5, 5, 5, pixel_color);
    } else {
        gdispGFillArea(vfx_gdisp, (pixel_x - 32) * 5, (pixel_y + 8) * 5, 5, 5, pixel_color);
    }
#else
    if (pixel_x <= 31) {
        gdispGFillArea(vfx_gdisp, pixel_x * 7 + 8, pixel_y * 7 + 12, 7, 7, pixel_color);
    } else {
        gdispGFillArea(vfx_gdisp, (pixel_x - 32) * 7 + 8, (pixel_y + 8) * 7 + 12, 7, 7, pixel_color);
    }
#endif
}

void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, float color_h, float color_l)
{
    for (uint8_t i=0; i<cx; i++) {
        for (uint8_t j=0; j<cy; j++) {
            for (uint8_t k=0; k<cz; k++) {
                vfx_draw_pixel(x+i, y+j, z+k, color_h, color_l);
            }
        }
    }
}

void vfx_draw_cube_bitmap(const uint8_t *bitmap, float color_l)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    static uint16_t color_pre = 0;
    static uint16_t color_h = 0;

    color_pre = color_h;
    for (uint8_t i=0; i<64; i++) {
        uint8_t temp = *(bitmap + i);

        for (uint8_t j=0; j<8; j++) {
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

    if ((color_h = color_pre + 1) == 512) {
        color_h = 0;
    }
}

void vfx_draw_layer_bitmap(uint8_t layer, const uint8_t *bitmap, float color_l)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = layer;
    static uint16_t color_pre = 0;
    static uint16_t color_h = 0;

    color_pre = color_h;
    for (uint8_t i=0; i<8; i++) {
        uint8_t temp = *(bitmap + i);

        for (uint8_t j=0; j<8; j++) {
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

    if ((color_h = color_pre + 1) == 512) {
        color_h = 0;
    }
}

void vfx_draw_layer_number(uint8_t num, uint8_t layer, float color_h, float color_l)
{
    uint8_t x = 0;
    uint8_t y = layer;
    uint8_t z = 0;

    for (uint8_t i=0; i<8; i++) {
        unsigned char temp = vfx_bitmap_number[num][i];

        for (uint8_t j=0; j<8; j++) {
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
#endif // CONFIG_SCREEN_PANEL_OUTPUT_VFX
