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

typedef enum {
#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
    VFX_MODE_IDX_12_BAND_R    = 0x0A,
    VFX_MODE_IDX_12_BAND_G    = 0x0B,
    VFX_MODE_IDX_SPECTRUM_R_N = 0x0C,
    VFX_MODE_IDX_SPECTRUM_G_N = 0x0D,
    VFX_MODE_IDX_SPECTRUM_M_N = 0x0E,
    VFX_MODE_IDX_SPECTRUM_R_L = 0x0F,
    VFX_MODE_IDX_SPECTRUM_G_L = 0x10,
    VFX_MODE_IDX_SPECTRUM_M_L = 0x11,
#else
    VFX_MODE_IDX_RANDOM       = 0x00,
    VFX_MODE_IDX_RAINBOW      = 0x01,
    VFX_MODE_IDX_RIBBON       = 0x02,
    VFX_MODE_IDX_GRADUAL      = 0x03,
    VFX_MODE_IDX_BREATHING    = 0x04,
    VFX_MODE_IDX_STARSKY_R    = 0x05,
    VFX_MODE_IDX_STARSKY_G    = 0x06,
    VFX_MODE_IDX_STARSKY_B    = 0x07,
    VFX_MODE_IDX_NUMBERS_S    = 0x08,
    VFX_MODE_IDX_NUMBERS_D    = 0x09,
    VFX_MODE_IDX_MAGIC_CARPET = 0x0A,
    VFX_MODE_IDX_ROTATING_F   = 0x0B,
    VFX_MODE_IDX_ROTATING_B   = 0x0C,
    VFX_MODE_IDX_FOUNTAIN_S_N = 0x0D,
    VFX_MODE_IDX_FOUNTAIN_G_N = 0x0E,
    VFX_MODE_IDX_FOUNTAIN_H_N = 0x0F,
    VFX_MODE_IDX_FOUNTAIN_S_L = 0x10,
    VFX_MODE_IDX_FOUNTAIN_G_L = 0x11,
    VFX_MODE_IDX_FOUNTAIN_H_L = 0x12,
#endif

    VFX_MODE_IDX_MAX,

    VFX_MODE_IDX_PAUSE = 0xFE,
    VFX_MODE_IDX_OFF   = 0xFF
} vfx_mode_t;

typedef struct {
    vfx_mode_t mode;
    uint16_t scale_factor;
    uint16_t lightness;
    uint8_t backlight;
} vfx_config_t;

#if defined(CONFIG_VFX_OUTPUT_ST7735) || defined(CONFIG_VFX_OUTPUT_ST7789)
    #define DEFAULT_VFX_MODE            VFX_MODE_IDX_12_BAND_G
    #define DEFAULT_VFX_SCALE_FACTOR    0x00FF
    #define DEFAULT_VFX_LIGHTNESS       0x00FF
    #define DEFAULT_VFX_BACKLIGHT       0xFF
#else
    #define DEFAULT_VFX_MODE            VFX_MODE_IDX_FOUNTAIN_H_L
    #define DEFAULT_VFX_SCALE_FACTOR    0x00FF
    #define DEFAULT_VFX_LIGHTNESS       0x006F
    #define DEFAULT_VFX_BACKLIGHT       0x00
#endif

extern GDisplay *vfx_gdisp;

extern void vfx_set_conf(vfx_config_t *cfg);
extern vfx_config_t *vfx_get_conf(void);

extern void vfx_init(void);

#endif /* INC_USER_VFX_H_ */
