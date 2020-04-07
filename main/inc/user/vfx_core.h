/*
 * vfx_core.h
 *
 *  Created on: 2019-07-03 20:05
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_CORE_H_
#define INC_USER_VFX_CORE_H_

#include <stdint.h>

extern uint32_t vfx_get_color(uint16_t color_idx, uint16_t color_ctr);

extern void vfx_draw_pixel(uint8_t x, uint8_t y, uint8_t z, uint16_t color_idx, uint16_t color_ctr);
extern void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, uint16_t color_idx, uint16_t color_ctr);
extern void vfx_draw_cube_bitmap(const uint8_t *bitmap, uint16_t color_ctr);
extern void vfx_draw_layer_bitmap(uint8_t layer, const uint8_t *bitmap, uint16_t color_ctr);
extern void vfx_draw_layer_number(uint8_t num, uint8_t layer, uint16_t color_idx, uint16_t color_ctr);

#endif /* INC_USER_VFX_CORE_H_ */
