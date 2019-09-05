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
#include "user/vfx_color_table.h"

inline uint32_t vfx_read_color_from_table(uint16_t color_idx, uint16_t color_ctr)
{
    uint16_t table_x = color_idx;
    uint16_t table_y = color_ctr;
    uint8_t *pixel_addr = (uint8_t *)vfx_color_table_512 + (table_x + table_y * 512) * 3;
    uint32_t pixel_color = *(pixel_addr + 0) << 16 | *(pixel_addr + 1) << 8 | *(pixel_addr + 2);
    return pixel_color;
}

#ifndef CONFIG_SCREEN_PANEL_OUTPUT_FFT
void vfx_draw_pixel(uint8_t x, uint8_t y, uint8_t z, uint16_t color_idx, uint16_t color_ctr)
{
    uint32_t pixel_color = vfx_read_color_from_table(color_idx, color_ctr);
    uint8_t pixel_x = x + y * 8;
    uint8_t pixel_y = z;
    GDisplay *g = gdispGetDisplay(0);
#ifdef CONFIG_VFX_OUTPUT_CUBE0414
    gdispGDrawPixel(g, pixel_x, pixel_y, pixel_color);
#elif defined(CONFIG_VFX_OUTPUT_ST7735)
    if (pixel_x <= 31) {
        gdispGFillArea(g, pixel_x * 5, pixel_y * 5, 5, 5, pixel_color);
    } else {
        gdispGFillArea(g, (pixel_x - 32) * 5, (pixel_y + 8) * 5, 5, 5, pixel_color);
    }
#else
    if (pixel_x <= 31) {
        gdispGFillArea(g, pixel_x * 7 + 8, pixel_y * 7 + 12, 7, 7, pixel_color);
    } else {
        gdispGFillArea(g, (pixel_x - 32) * 7 + 8, (pixel_y + 8) * 7 + 12, 7, 7, pixel_color);
    }
#endif
}

void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, uint16_t color_idx, uint16_t color_ctr)
{
    for (uint8_t i=0; i<cx; i++) {
        for (uint8_t j=0; j<cy; j++) {
            for (uint8_t k=0; k<cz; k++) {
                vfx_draw_pixel(x+i, y+j, z+k, color_idx, color_ctr);
            }
        }
    }
}

void vfx_draw_cube_bitmap(const uint8_t *bitmap)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    static uint16_t color_pre = 0;
    static uint16_t color_idx = 0;
    uint16_t color_ctr = vfx_ctr;
    color_pre = color_idx;
    for (uint8_t i=0; i<64; i++) {
        uint8_t temp = *(bitmap + i);
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_idx, color_ctr);
            } else {
                vfx_draw_pixel(x, y, z, 0, 511);
            }
            temp <<= 1;
            if (z++ == 7) {
                z = 0;
                if (x++ == 7) {
                    x = 0;
                    if (y++ == 7) {
                        y = 0;
                    }
                }
                if (++color_idx > 511) {
                    color_idx = 0;
                }
            }
        }
    }
    if (color_pre == 510) {
        color_idx = 0;
    } else {
        color_idx = color_pre + 2;
    }
}

void vfx_draw_layer_bitmap(uint8_t layer, const uint8_t *bitmap)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = layer;
    static uint16_t color_pre = 0;
    static uint16_t color_idx = 0;
    uint16_t color_ctr = vfx_ctr;
    color_pre = color_idx;
    for (uint8_t i=0; i<8; i++) {
        uint8_t temp = *(bitmap + i);
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_idx, color_ctr);
            } else {
                vfx_draw_pixel(x, y, z, 0, 511);
            }
            temp <<= 1;
            if (y++ == 7) {
                y = 0;
                if (x++ == 7) {
                    x = 0;
                }
            }
            if (++color_idx > 511) {
                color_idx = 0;
            }
        }
    }
    if (color_pre == 511) {
        color_idx = 0;
    } else {
        color_idx = color_pre + 1;
    }
}

void vfx_draw_layer_number(uint8_t num, uint8_t layer, uint16_t color_idx, uint16_t color_ctr)
{
    uint8_t x = 0;
    uint8_t y = layer;
    uint8_t z = 0;
    for (uint8_t i=0; i<8; i++) {
        unsigned char temp = vfx_bitmap_number[num][i];
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                vfx_draw_pixel(x, y, z, color_idx, color_ctr);
            } else {
                vfx_draw_pixel(x, y, z, 0, 511);
            }
            temp <<= 1;
            if (z++ == 7) {
                z = 0;
                if (x++ == 7) {
                    x = 7;
                }
            }
        }
    }
}
#endif // CONFIG_SCREEN_PANEL_OUTPUT_FFT
