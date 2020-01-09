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

#define DEFAULT_VFX_MODE        0x0F
#define DEFAULT_VFX_SCALE       192
#ifndef CONFIG_VFX_OUTPUT_CUBE0414
    #define DEFAULT_VFX_CONTRAST    0x0100
#else
    #define DEFAULT_VFX_CONTRAST    0x0190
#endif
#define DEFAULT_VFX_BACKLIGHT   255

struct vfx_conf {
    uint8_t mode;
    uint16_t scale;
    uint16_t contrast;
    uint8_t backlight;
};

extern GDisplay *vfx_gdisp;
extern fft_config_t *vfx_fft_plan;

extern void vfx_set_conf(struct vfx_conf *cfg);
extern struct vfx_conf *vfx_get_conf(void);

extern void vfx_init(void);

#endif /* INC_USER_VFX_H_ */
