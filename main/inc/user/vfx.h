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

extern GDisplay *vfx_gdisp;
extern fft_config_t *vfx_fft_plan;

extern void vfx_set_mode(uint8_t idx);
extern uint8_t vfx_get_mode(void);

extern void vfx_set_scale(uint16_t val);
extern uint16_t vfx_get_scale(void);

extern void vfx_set_contrast(uint16_t val);
extern uint16_t vfx_get_contrast(void);

extern void vfx_set_backlight(uint8_t val);
extern uint8_t vfx_get_backlight(void);

extern void vfx_init(void);

#endif /* INC_USER_VFX_H_ */
