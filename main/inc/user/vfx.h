/*
 * vfx.h
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_H_
#define INC_USER_VFX_H_

#include <stdint.h>

#include "gfx.h"
#include "fft.h"

#define FFT_N 128

#define DEFAULT_VFX_MODE 0x0F
#define DEFAULT_VFX_SCALE_FACTOR 0xFF

#ifndef CONFIG_VFX_OUTPUT_CUBE0414
    #define DEFAULT_VFX_LIGHTNESS 0x00FF
#else
    #define DEFAULT_VFX_LIGHTNESS 0x006F
#endif

#define DEFAULT_VFX_BACKLIGHT 0xFF

enum vfx_mode {
    VFX_MODE_PAUSE = 0xFE,
    VFX_MODE_OFF   = 0xFF,
};

typedef struct {
    uint8_t mode;
    uint16_t scale_factor;
    uint16_t lightness;
    uint8_t backlight;
} vfx_config_t;

extern GDisplay *vfx_gdisp;

extern float vfx_fft_input[FFT_N];
extern float vfx_fft_output[FFT_N];

#ifdef CONFIG_SCREEN_PANEL_OUTPUT_VFX
    #ifdef CONFIG_VFX_OUTPUT_ST7735
        // ani0.gif
        extern const char ani0_160x80_gif_ptr[] asm("_binary_ani0_160x80_gif_start");
        extern const char ani0_160x80_gif_end[] asm("_binary_ani0_160x80_gif_end");
        // ani1.gif
        extern const char ani1_160x80_gif_ptr[] asm("_binary_ani1_160x80_gif_start");
        extern const char ani1_160x80_gif_end[] asm("_binary_ani1_160x80_gif_end");
    #elif defined(CONFIG_VFX_OUTPUT_ST7789)
        // ani0.gif
        extern const char ani0_240x135_gif_ptr[] asm("_binary_ani0_240x135_gif_start");
        extern const char ani0_240x135_gif_end[] asm("_binary_ani0_240x135_gif_end");
        // ani1.gif
        extern const char ani1_240x135_gif_ptr[] asm("_binary_ani1_240x135_gif_start");
        extern const char ani1_240x135_gif_end[] asm("_binary_ani1_240x135_gif_end");
    #endif
#endif

extern void vfx_set_conf(vfx_config_t *cfg);
extern vfx_config_t *vfx_get_conf(void);

extern void vfx_init(void);

#endif /* INC_USER_VFX_H_ */
