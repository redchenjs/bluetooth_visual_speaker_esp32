/*
 * vfx_core.h
 *
 *  Created on: 2019-07-03 20:05
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_CORE_H_
#define INC_USER_VFX_CORE_H_

#include <stdint.h>

extern uint32_t vfx_get_color(float color_h, float color_l);
extern void vfx_draw_pixel_raw(uint8_t x, uint8_t y, uint8_t z, uint32_t color);
extern void vfx_draw_pixel(uint8_t x, uint8_t y, uint8_t z, float color_h, float color_l);
extern void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, float color_h, float color_l);
extern void vfx_draw_cube_bitmap(const uint8_t *bitmap, float color_l);
extern void vfx_draw_layer_bitmap(uint8_t layer, const uint8_t *bitmap, float color_l);
extern void vfx_draw_layer_number(uint8_t num, uint8_t layer, float color_h, float color_l);

#endif /* INC_USER_VFX_CORE_H_ */
