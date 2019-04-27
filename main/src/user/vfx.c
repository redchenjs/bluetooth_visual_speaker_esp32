/*
 * vfx.c
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <math.h>

#include "esp_log.h"

#include "fft.h"
#include "gfx.h"

#include "os/event.h"
#include "user/color_table.h"
#include "user/bitmap.h"
#include "user/fifo.h"
#include "user/vfx.h"

#define TAG "vfx"

static uint8_t vfx_mode = 13;

static uint32_t read_color_from_table(uint16_t color_idx, uint16_t color_lum)
{
    uint16_t table_x = color_idx;
    uint16_t table_y = color_lum;
    uint8_t *pixel_addr = (uint8_t *)color_table_512 + (table_x + table_y * 512) * 3;
    uint32_t pixel_color = *(pixel_addr + 0) << 16 | *(pixel_addr + 1) << 8 | *(pixel_addr + 2);
    return pixel_color;
}

static void write_pixel(uint8_t x, uint8_t y, uint8_t z,
                        uint16_t color_idx, uint16_t color_lum)
{
    uint32_t pixel_color = read_color_from_table(color_idx, color_lum);
    uint8_t pixel_x = x + y * 8;
    uint8_t pixel_y = z;
    GDisplay *g = gdispGetDisplay(0);
    gdispGDrawPixel(g, pixel_x, pixel_y, pixel_color);
}

static void fill_cube(uint8_t x0, uint8_t y0, uint8_t z0,
                      uint8_t x1, uint8_t y1, uint8_t z1,
                      uint16_t color_idx, uint16_t color_lum)
{
    for (uint8_t x=x0; x<=x1; x++) {
        for (uint8_t y=y0; y<=y1; y++) {
            for (uint8_t z=z0; z<=z1; z++) {
                write_pixel(x, y, z, color_idx, color_lum);
            }
        }
    }
}

static void write_cube_bitmap(uint8_t x0, uint8_t y0, uint8_t z0,
                              uint8_t x1, uint8_t y1, uint8_t z1,
                              const uint8_t *bitmap)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;
    static uint16_t color_pre = 0;
    static uint16_t color_idx = 0;
    static uint16_t color_lum = 400;
    color_pre = color_idx;
    for (uint8_t i=0; i<64; i++) {
        uint8_t temp = *(bitmap + i);
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                write_pixel(x, y, z, color_idx, color_lum);
            } else {
                write_pixel(x, y, z, 0, 511);
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
                if (color_idx++ == 511) {
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

static void write_layer_bitmap(uint8_t layer, const uint8_t *bitmap)
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = layer;
    static uint16_t color_pre = 0;
    static uint16_t color_idx = 0;
    static uint16_t color_lum = 400;
    color_pre = color_idx;
    for (uint8_t i=0; i<8; i++) {
        uint8_t temp = *(bitmap + i);
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                write_pixel(x, y, z, color_idx, color_lum);
            } else {
                write_pixel(x, y, z, 0, 511);
            }
            temp <<= 1;
            if (y++ == 7) {
                y = 0;
                if (x++ == 7) {
                    x = 0;
                }
            }
            if (color_idx++ == 511) {
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

static void write_layer_number(uint8_t num, uint8_t layer, uint16_t color_idx, uint16_t color_lum)
{
    uint8_t x = 0;
    uint8_t y = layer;
    uint8_t z = 0;
    for (uint8_t i=0; i<8; i++) {
        unsigned char temp = bitmap_number[num][i];
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                write_pixel(x, y, z, color_idx, color_lum);
            } else {
                write_pixel(x, y, z, 0, 511);
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

static void clear_cube(void)
{
    GDisplay *g = gdispGetDisplay(0);
    gdispGFillArea(g, 0, 0, 64, 8, 0x000000);
}

void vfx_task(void *pvParameter)
{
    fifo_init();

    gfxInit();

    while (1) {
        switch (vfx_mode) {
        case 2: {   // 流动炫彩灯 8*8*8
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                while (1) {
                    write_pixel(x, y, z, color_idx, color_lum);
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
                    if (color_idx++ == 511) {
                        color_idx = 0;
                    }
                }

                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 3: {   // 全彩渐变灯 8*8*8
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                fill_cube(0, 0, 0, 7, 7, 7, color_idx, color_lum);
                if (color_idx++ == 511) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 4: {   // 渐变呼吸灯 8*8*8
            uint8_t lum_dir = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 511;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                fill_cube(0, 0, 0, 7, 7, 7, color_idx, color_lum);
                if (lum_dir == 0) {     // 暗->明
                    if (color_lum-- == 256) {
                        color_lum = 256;
                        if (color_idx++ == 511) {
                            color_idx = 0;
                        }
                        lum_dir = 1;
                    }
                } else {    // 明->暗
                    if (color_lum++ == 511) {
                        color_lum = 511;
                        if (color_idx++ == 511) {
                            color_idx = 0;
                        }
                        lum_dir = 0;
                    }
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 5: {   // 渐变呼吸灯 随机
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t lum_max = 400;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_idx[512] = {0};

            for (uint16_t i=0; i<512; i++) {
                led_idx[i] = i;
                color_idx[i] = i % 80;
            }
            for (uint16_t i=0; i<511; i++) {
                uint16_t rnd = esp_random() % (512 - i - 1);
                uint16_t tmp = led_idx[rnd];
                led_idx[rnd] = led_idx[512 - i - 1];
                led_idx[512 - i - 1] = tmp;
            }
            for (uint16_t i=0; i<511; i++) {
                uint16_t rnd = esp_random() % (512 - i - 1);
                uint16_t tmp = color_idx[rnd];
                color_idx[rnd] = color_idx[512 - i - 1];
                color_idx[512 - i - 1] = tmp;
            }
            uint16_t i=0;
            while (1) {
                for (; i<512; i++) {
                    uint16_t j = 511;
                    uint16_t k = lum_max;
                    for (uint16_t n=lum_max; n<511; n++) {
                        x = (led_idx[i] % 64) % 8;
                        y = (led_idx[i] % 64) / 8;
                        z = led_idx[i] / 64;
                        write_pixel(x, y, z, color_idx[i], --j);
                        if (i >= led_num - 1) {
                            x = (led_idx[i-(led_num-1)] % 64) % 8;
                            y = (led_idx[i-(led_num-1)] % 64) / 8;
                            z = led_idx[i-(led_num-1)] / 64;
                            write_pixel(x, y, z, color_idx[i-(led_num-1)], ++k);
                        }
                        if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                            xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                            clear_cube();
                            goto exit;
                        }
                        gfxSleepMilliseconds(8);
                    }
                }
                for (i=0; i<led_num; i++) {
                    uint16_t j = 511;
                    uint16_t k = lum_max;
                    for (uint16_t n=lum_max; n<511; n++) {
                        x = (led_idx[i] % 64) % 8;
                        y = (led_idx[i] % 64) / 8;
                        z = led_idx[i] / 64;
                        write_pixel(x, y, z, color_idx[i], --j);
                        x = (led_idx[(512-(led_num-1))+i] % 64) % 8;
                        y = (led_idx[(512-(led_num-1))+i] % 64) / 8;
                        z = led_idx[(512-(led_num-1))+i] / 64;
                        write_pixel(x, y, z, color_idx[(512-(led_num-1))+i], ++k);
                        if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                            xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                            clear_cube();
                            goto exit;
                        }
                        gfxSleepMilliseconds(8);
                    }
                }
            }
exit:
            break;
        }
        case 6: {   // 数字渐变 0-9
            uint16_t num = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                write_layer_number(num, 3, color_idx, color_lum);
                write_layer_number(num, 4, color_idx, color_lum);
                write_layer_number(num, 5, color_idx, color_lum);
                if (color_idx++ == 511) {
                    color_idx = 0;
                }
                if (num++ == 9) {
                    num = 0;
                }
                gfxSleepMilliseconds(1000);
            }
            break;
        }
        case 7: {   // 流动炫彩灯 8*8 重复
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                uint16_t color_base = color_idx;
                while (1) {
                    for (uint8_t i=0; i<8; i++) {
                        write_pixel(x, y, i, color_idx, color_lum);
                    }
                    if (x++ == 7) {
                        x = 0;
                        if (y++ == 7) {
                            y = 0;
                            break;
                        }
                    }
                    if (color_idx++ == 511) {
                        color_idx = 0;
                    }
                }
                color_idx = ++color_base;
                if (color_idx == 512) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 8: {   // 数字移动 0-9
            uint16_t num = 0;
            uint16_t layer0 = 0;
            uint16_t layer1 = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                for (uint8_t i=layer0; i<=layer1; i++) {
                    write_layer_number(num, i, color_idx, color_lum);
                }
                if (layer1 != 7 && layer0 != 0) {
                    write_layer_number(num, layer0, 0, 511);
                    layer1++;
                    layer0++;
                } else if (layer1 == 7) {
                    if (layer0++ == 7) {
                        write_layer_number(num, 7, 0, 511);
                        layer0 = 0;
                        layer1 = 0;
                        if (num++ == 9) {
                            num = 0;
                        }
                    } else {
                        write_layer_number(num, layer0 - 1, 0, 511);
                    }
                } else {
                    if ((layer1 - layer0) != 3) {
                        layer1++;
                    } else {
                        write_layer_number(num, 0, 0, 511);
                        layer1++;
                        layer0++;
                    }
                }
                if (color_idx++ == 511) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(100);
            }
            break;
        }
        case 9: {   // 炫彩渐变波形
            uint16_t frame_idx = 0;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                write_cube_bitmap(0, 0, 0, 7, 7, 7, bitmap_wave[frame_idx]);
                if (frame_idx++ == 44) {
                    frame_idx = 8;
                }
                gfxSleepMilliseconds(20);
            }
            break;
        }
        case 10: {   // 炫彩螺旋线条 逆时针旋转
            uint16_t frame_pre = 0;
            uint16_t frame_idx = 0;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                frame_pre = frame_idx;
                for (uint8_t i=0; i<8; i++) {
                    write_layer_bitmap(i, bitmap_line[frame_idx]);
                    if (frame_idx++ == 27) {
                        frame_idx = 0;
                    }
                }
                if (frame_pre == 27) {
                    frame_idx = 0;
                } else {
                    frame_idx = frame_pre + 1;
                }
                gfxSleepMilliseconds(40);
            }
            break;
        }
        case 11: {   // 炫彩螺旋线条 顺时针旋转
            uint16_t frame_pre = 0;
            uint16_t frame_idx = 0;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                frame_pre = frame_idx;
                for (uint8_t i=0; i<8; i++) {
                    write_layer_bitmap(i, bitmap_line[frame_idx]);
                    if (frame_idx-- == 0) {
                        frame_idx = 27;
                    }
                }
                if (frame_pre == 0) {
                    frame_idx = 27;
                } else {
                    frame_idx = frame_pre - 1;
                }
                gfxSleepMilliseconds(40);
            }
            break;
        }
        case 12: {   // 音频FFT 横排彩虹
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_idx = 0;
            uint16_t color_lum = 400;
            uint16_t fft_amp[64] = {0};
            fft_config_t *fft_plan = fft_init(128, FFT_COMPLEX, FFT_FORWARD, NULL, NULL);
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }

                for (uint16_t k=0; k<128; k++) {
                    fft_plan->input[2*k] = (float)fifo_read();
                    fft_plan->input[2*k+1] = 0.0;
                }

                fft_execute(fft_plan);

                for (uint16_t k=0; k<64; k++) {
                    fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2));
                }

                color_idx = 63;
                for (uint16_t i=0; i<64; i++) {
                    uint8_t temp = fft_amp[i] / 8192;
                    if (temp != 0) {
                        fill_cube(x, 7-y, 0, x, 7-y, 7-temp, 0, 511);
                        fill_cube(x, 7-y, 7-temp, x, 7-y, 7, color_idx, color_lum);
                    } else {
                        fill_cube(x, 7-y, 0, x, 7-y, 6, 0, 511);
                        fill_cube(x, 7-y, 7, x, 7-y, 7, color_idx, color_lum);
                    }

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    color_idx += 7;
                    if (color_idx == 511) {
                        color_idx = 7;
                    }
                }

                gfxSleepMilliseconds(30);
            }
            fft_destroy(fft_plan);
            break;
        }
        case 13: {   // 音频FFT 螺旋彩虹
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_idx[64] = {0};
            uint16_t color_lum[64] = {400};
            uint16_t fft_amp[64] = {0};
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
            for (uint16_t i=0; i<64; i++) {
                color_idx[i] = i * 8;
                color_lum[i] = 400;
            }
            fft_config_t *fft_plan = fft_init(128, FFT_COMPLEX, FFT_FORWARD, NULL, NULL);
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }

                for (uint16_t k=0; k<128; k++) {
                    fft_plan->input[2*k] = (float)fifo_read();
                    fft_plan->input[2*k+1] = 0.0;
                }

                fft_execute(fft_plan);

                for (uint16_t k=0; k<64; k++) {
                    fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2));
                }

                for (uint16_t i=0; i<64; i++) {
                    x = led_idx_table[0][i];
                    y = led_idx_table[1][i];

                    uint8_t temp = fft_amp[i] / 8192;
                    if (temp != 0) {
                        fill_cube(x, 7-y, 0, x, 7-y, 7-temp, 0, 511);
                        fill_cube(x, 7-y, 7-temp, x, 7-y, 7, color_idx[i], color_lum[i]);
                    } else {
                        fill_cube(x, 7-y, 0, x, 7-y, 6, 0, 511);
                        fill_cube(x, 7-y, 7, x, 7-y, 7, color_idx[i], color_lum[i]);
                    }

                    if (color_idx[i]-- == 0) {
                        color_idx[i] = 511;
                    }
                }

                for (uint16_t i=0; i<64; i++) {
                    if (color_idx[i]-- == 0) {
                        color_idx[i] = 511;
                    }
                }

                gfxSleepMilliseconds(30);
            }
            fft_destroy(fft_plan);
            break;
        }
        default:
            clear_cube();
            gfxSleepMilliseconds(100);
            break;
        }
    }
}

void vfx_set_mode(uint8_t mode)
{
    vfx_mode = mode;
    xEventGroupSetBits(user_event_group, VFX_RELOAD_BIT);
}

uint8_t vfx_get_mode(void)
{
    return vfx_mode;
}
