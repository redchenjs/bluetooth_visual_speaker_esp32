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

static uint8_t vfx_mode = 0x0F;
static uint16_t vfx_ctr = 0x0190;

static uint32_t read_color_from_table(uint16_t color_idx, uint16_t color_ctr)
{
    uint16_t table_x = color_idx;
    uint16_t table_y = color_ctr;
    uint8_t *pixel_addr = (uint8_t *)color_table_512 + (table_x + table_y * 512) * 3;
    uint32_t pixel_color = *(pixel_addr + 0) << 16 | *(pixel_addr + 1) << 8 | *(pixel_addr + 2);
    return pixel_color;
}

#if !defined(CONFIG_SCREEN_PANEL_OUTPUT_FFT)
static void write_pixel(uint8_t x, uint8_t y, uint8_t z,
                        uint16_t color_idx, uint16_t color_ctr)
{
    uint32_t pixel_color = read_color_from_table(color_idx, color_ctr);
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

static void fill_cube(uint8_t x,  uint8_t y,  uint8_t z,
                      uint8_t cx, uint8_t cy, uint8_t cz,
                      uint16_t color_idx, uint16_t color_ctr)
{
    for (uint8_t i=0; i<cx; i++) {
        for (uint8_t j=0; j<cy; j++) {
            for (uint8_t k=0; k<cz; k++) {
                write_pixel(x+i, y+j, z+k, color_idx, color_ctr);
            }
        }
    }
}

static void write_cube_bitmap(const uint8_t *bitmap)
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
                write_pixel(x, y, z, color_idx, color_ctr);
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

static void write_layer_bitmap(uint8_t layer, const uint8_t *bitmap)
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
                write_pixel(x, y, z, color_idx, color_ctr);
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

static void write_layer_number(uint8_t num, uint8_t layer, uint16_t color_idx, uint16_t color_ctr)
{
    uint8_t x = 0;
    uint8_t y = layer;
    uint8_t z = 0;
    for (uint8_t i=0; i<8; i++) {
        unsigned char temp = bitmap_number[num][i];
        for (uint8_t j=0; j<8; j++) {
            if (temp & 0x80) {
                write_pixel(x, y, z, color_idx, color_ctr);
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
#endif // defined(VFX_OUTPUT_CUBE0414) || defined(SCREEN_PANEL_OUTPUT_MMAP)

static void clear_cube(void)
{
    GDisplay *g = gdispGetDisplay(0);
#ifdef CONFIG_VFX_OUTPUT_CUBE0414
    gdispGFillArea(g, 0, 0, 64, 8, 0x000000);
#elif defined(CONFIG_VFX_OUTPUT_ST7735)
    gdispGFillArea(g, 0, 0, 160, 80, 0x000000);
#else
    gdispGFillArea(g, 0, 0, 240, 135, 0x000000);
#endif
}

static void vfx_task_handle(void *pvParameter)
{
    gfxInit();

#if defined(CONFIG_VFX_OUTPUT_CUBE0414)
    ESP_LOGI(TAG, "Start Light Cube Output");
#elif defined(CONFIG_SCREEN_PANEL_OUTPUT_MMAP)
    vfx_ctr = 0x0100;
    ESP_LOGI(TAG, "Start LCD MMAP Output");
#else
    vfx_ctr = 0x0100;
    ESP_LOGI(TAG, "Start LCD FFT Output");
#endif

    while (1) {
#if defined(CONFIG_SCREEN_PANEL_OUTPUT_FFT)
        // LCD FFT Output
        uint16_t color_tmp = 0;
        uint16_t color_idx = 0;
        uint16_t color_ctr = vfx_ctr;
        float fft_amp[64] = {0};
        GDisplay *g = gdispGetDisplay(0);

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

            fft_amp[0] = sqrt(pow(fft_plan->output[0], 2) + pow(fft_plan->output[1], 2)) / 128;
            for (uint16_t k=1; k<64; k++) {
                fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2)) / 128 * 2;
            }

            color_tmp = color_idx;
#if defined(CONFIG_VFX_OUTPUT_ST7735)
            for (uint16_t i=0; i<52; i++) {
                uint16_t temp = fft_amp[i] / 8.192;
                uint32_t pixel_color = read_color_from_table(color_idx, color_ctr);

                if (temp > 80) {
                    temp = 80;
                } else if (temp < 1) {
                    temp = 1;
                }

                uint16_t clear_x  = i * 3 + 2;
                uint16_t clear_cx = 3;
                uint16_t clear_y  = 0;
                uint16_t clear_cy = 80 - temp;

                uint16_t fill_x  = i * 3 + 2;
                uint16_t fill_cx = 3;
                uint16_t fill_y  = 80 - temp;
                uint16_t fill_cy = temp;
#else
            for (uint16_t i=0; i<60; i++) {
                uint16_t temp = fft_amp[i] / 4.8;
                uint32_t pixel_color = read_color_from_table(color_idx, color_ctr);

                if (temp > 135) {
                    temp = 135;
                } else if (temp < 1) {
                    temp = 1;
                }

                uint16_t clear_x  = i * 4;
                uint16_t clear_cx = 4;
                uint16_t clear_y  = 0;
                uint16_t clear_cy = 135 - temp;

                uint16_t fill_x  = i * 4;
                uint16_t fill_cx = 4;
                uint16_t fill_y  = 135 - temp;
                uint16_t fill_cy = temp;
#endif
                gdispGFillArea(g, clear_x, clear_y, clear_cx, clear_cy, 0x000000);
                gdispGFillArea(g, fill_x, fill_y, fill_cx, fill_cy, pixel_color);

                if (++color_idx > 511) {
                    color_idx = 0;
                }
            }

            color_idx = ++color_tmp;
            if (color_idx > 511) {
                color_idx = 0;
            }

            gfxSleepMilliseconds(16);
        }
        fft_destroy(fft_plan);
#else
        // Light Cube Output
        switch (vfx_mode) {
        case 0x01: {   // 流动炫彩灯 8*8*8
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                while (1) {
                    write_pixel(x, y, z, color_idx, color_ctr);
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
                    if (++color_idx > 511) {
                        color_idx = 0;
                    }
                }

                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 0x02: {   // 流动炫彩灯 8*8 重复
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_tmp = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                color_tmp = color_idx;
                while (1) {
                    for (uint8_t i=0; i<8; i++) {
                        write_pixel(x, y, i, color_idx, color_ctr);
                    }
                    if (x++ == 7) {
                        x = 0;
                        if (y++ == 7) {
                            y = 0;
                            break;
                        }
                    }
                    if (++color_idx > 511) {
                        color_idx = 0;
                    }
                }
                color_idx = ++color_tmp;
                if (color_idx > 511) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 0x03: {   // 全彩渐变灯 8*8*8
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                fill_cube(0, 0, 0, 8, 8, 8, color_idx, color_ctr);
                if (++color_idx > 511) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 0x04: {   // 渐变呼吸灯 8*8*8
            uint8_t ctr_dir = 0;
            uint8_t color_cnt = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = 511;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                fill_cube(0, 0, 0, 8, 8, 8, color_idx, color_ctr);
                if (ctr_dir == 0) {     // 暗->明
                    if (color_ctr-- == vfx_ctr) {
                        color_ctr = vfx_ctr;
                        ctr_dir = 1;
                    }
                } else {    // 明->暗
                    if (++color_ctr > 511) {
                        color_ctr = 511;
                        ctr_dir = 0;
                    }
                }
                if (++color_cnt % 16 == 0) {
                    if (++color_idx > 511) {
                        color_idx = 0;
                    }
                }
                gfxSleepMilliseconds(8);
            }
            break;
        }
        case 0x05: {   // 渐变呼吸灯 随机 紫-红
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_ctr = vfx_ctr;
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
            break;
        }
        case 0x06: {   // 渐变呼吸灯 随机 青-蓝
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_ctr = vfx_ctr;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_idx[512] = {0};

            for (uint16_t i=0; i<512; i++) {
                led_idx[i] = i;
                color_idx[i] = i % 80 + 170;
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
            break;
        }
        case 0x07: {   // 渐变呼吸灯 随机 黄-绿
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t z = 0;
            uint16_t led_ctr = vfx_ctr;
            uint16_t led_num = 32;
            uint16_t led_idx[512] = {0};
            uint16_t color_idx[512] = {0};

            for (uint16_t i=0; i<512; i++) {
                led_idx[i] = i;
                color_idx[i] = i % 85 + 345;
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
                    uint16_t k = led_ctr;
                    for (uint16_t n=led_ctr; n<511; n++) {
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
            break;
        }
        case 0x08: {   // 数字渐变 0-9
            uint16_t num = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                write_layer_number(num, 2, color_idx, color_ctr);
                write_layer_number(num, 3, color_idx, color_ctr);
                write_layer_number(num, 4, color_idx, color_ctr);
                write_layer_number(num, 5, color_idx, color_ctr);
                color_idx += 8;
                if (color_idx > 511) {
                    color_idx = 0;
                }
                if (++num > 9) {
                    num = 0;
                }
                gfxSleepMilliseconds(1000);
            }
            break;
        }
        case 0x09: {   // 数字移动 0-9
            uint16_t num = 0;
            uint16_t layer0 = 0;
            uint16_t layer1 = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                for (uint8_t i=layer0; i<=layer1; i++) {
                    write_layer_number(num, i, color_idx, color_ctr);
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
                    if ((layer1 - layer0) != 4) {
                        layer1++;
                    } else {
                        write_layer_number(num, 0, 0, 511);
                        layer1++;
                        layer0++;
                    }
                }
                if (++color_idx > 511) {
                    color_idx = 0;
                }
                gfxSleepMilliseconds(80);
            }
            break;
        }
        case 0x0A: {   // 炫彩渐变波动
            uint16_t frame_idx = 0;
            while (1) {
                if (xEventGroupGetBits(user_event_group) & VFX_RELOAD_BIT) {
                    xEventGroupClearBits(user_event_group, VFX_RELOAD_BIT);
                    clear_cube();
                    break;
                }
                write_cube_bitmap(bitmap_wave[frame_idx]);
                if (frame_idx++ == 44) {
                    frame_idx = 8;
                }
                gfxSleepMilliseconds(16);
            }
            break;
        }
        case 0x0B: {   // 炫彩螺旋线条 逆时针旋转
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
        case 0x0C: {   // 炫彩螺旋线条 顺时针旋转
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
        case 0x0D: {   // 音频FFT 横排彩虹(静止)
            uint8_t x = 0;
            uint8_t y = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            float fft_amp[64] = {0};
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

                fft_amp[0] = sqrt(pow(fft_plan->output[0], 2) + pow(fft_plan->output[1], 2)) / 128;
                for (uint16_t k=1; k<64; k++) {
                    fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2)) / 128 * 2;
                }

                color_idx = 63;
                for (uint16_t i=0; i<64; i++) {
                    uint8_t temp = fft_amp[i] / 81.92;

                    if (temp > 8) {
                        temp = 8;
                    } else if (temp < 1) {
                        temp = 1;
                    }

                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = 8 - temp;

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = 8 - temp;
                    uint8_t fill_cz = temp;

                    fill_cube(clear_x, clear_y, clear_z,
                              clear_cx, clear_cy, clear_cz,
                              0, 511);
                    fill_cube(fill_x, fill_y, fill_z,
                              fill_cx, fill_cy, fill_cz,
                              color_idx, color_ctr);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    color_idx += 7;
                    if (color_idx > 511) {
                        color_idx = 7;
                    }
                }

                gfxSleepMilliseconds(16);
            }
            fft_destroy(fft_plan);
            break;
        }
        case 0x0E: {   // 音频FFT 横排彩虹(流动)
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_cnt = 0;
            uint16_t color_tmp = 0;
            uint16_t color_idx = 0;
            uint16_t color_ctr = vfx_ctr;
            float fft_amp[64] = {0};
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

                fft_amp[0] = sqrt(pow(fft_plan->output[0], 2) + pow(fft_plan->output[1], 2)) / 128;
                for (uint16_t k=1; k<64; k++) {
                    fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2)) / 128 * 2;
                }

                color_idx = color_tmp;
                for (uint16_t i=0; i<64; i++) {
                    uint8_t temp = fft_amp[i] / 81.92;

                    if (temp > 8) {
                        temp = 8;
                    } else if (temp < 1) {
                        temp = 1;
                    }

                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = 8 - temp;

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = 8 - temp;
                    uint8_t fill_cz = temp;

                    fill_cube(clear_x, clear_y, clear_z,
                              clear_cx, clear_cy, clear_cz,
                              0, 511);
                    fill_cube(fill_x, fill_y, fill_z,
                              fill_cx, fill_cy, fill_cz,
                              color_idx, color_ctr);

                    if (y++ == 7) {
                        y = 0;
                        if (x++ == 7) {
                            x = 0;
                        }
                    }

                    color_idx += 8;
                    if (color_idx > 511) {
                        color_idx = 0;
                    }
                }

                if (++color_cnt % 8 == 0) {
                    color_tmp += 8;
                    if (color_tmp > 511) {
                        color_tmp = 0;
                    }
                }

                gfxSleepMilliseconds(16);
            }
            fft_destroy(fft_plan);
            break;
        }
        case 0x0F: {   // 音频FFT 螺旋彩虹
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t color_flg = 0;
            uint8_t color_cnt = 0;
            uint16_t color_idx[64] = {0};
            uint16_t color_ctr[64] = {vfx_ctr};
            float fft_amp[64] = {0};
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
                color_ctr[i] = vfx_ctr;
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

                fft_amp[0] = sqrt(pow(fft_plan->output[0], 2) + pow(fft_plan->output[1], 2)) / 128;
                for (uint16_t k=1; k<64; k++) {
                    fft_amp[k] = sqrt(pow(fft_plan->output[2*k], 2) + pow(fft_plan->output[2*k+1], 2)) / 128 * 2;
                }

                for (uint16_t i=0; i<64; i++) {
                    x = led_idx_table[0][i];
                    y = led_idx_table[1][i];

                    uint8_t temp = fft_amp[i] / 81.92;

                    if (temp > 8) {
                        temp = 8;
                    } else if (temp < 1) {
                        temp = 1;
                    }

                    uint8_t clear_x  = x;
                    uint8_t clear_cx = 1;
                    uint8_t clear_y  = 7 - y;
                    uint8_t clear_cy = 1;
                    uint8_t clear_z  = 0;
                    uint8_t clear_cz = 8 - temp;

                    uint8_t fill_x  = x;
                    uint8_t fill_cx = 1;
                    uint8_t fill_y  = 7 - y;
                    uint8_t fill_cy = 1;
                    uint8_t fill_z  = 8 - temp;
                    uint8_t fill_cz = temp;

                    fill_cube(clear_x, clear_y, clear_z,
                              clear_cx, clear_cy, clear_cz,
                              0, 511);
                    fill_cube(fill_x, fill_y, fill_z,
                              fill_cx, fill_cy, fill_cz,
                              color_idx[i], color_ctr[i]);

                    if (color_flg) {
                        if (color_idx[i]-- == 0) {
                            color_idx[i] = 511;
                        }
                    }
                }

                if (++color_cnt % 2 == 0) {
                    color_flg = 1;
                } else {
                    color_flg = 0;
                }

                gfxSleepMilliseconds(16);
            }
            fft_destroy(fft_plan);
            break;
        }
        default:
            clear_cube();
            gfxSleepMilliseconds(1000);
exit:
            break;
        }
#endif // CONFIG_SCREEN_PANEL_OUTPUT_FFT
    }
}

void vfx_set_mode(uint8_t mode)
{
#ifdef CONFIG_ENABLE_VFX
    vfx_mode = mode;
    xEventGroupSetBits(user_event_group, VFX_RELOAD_BIT);
#endif
}

uint8_t vfx_get_mode(void)
{
    return vfx_mode;
}

void vfx_set_ctr(uint16_t ctr)
{
    vfx_ctr = ctr;
    xEventGroupSetBits(user_event_group, VFX_RELOAD_BIT);
}

uint16_t vfx_get_ctr(void)
{
    return vfx_ctr;
}

void vfx_init(void)
{
    xTaskCreate(vfx_task_handle, "vfxT", 4096, NULL, 5, NULL);
}
