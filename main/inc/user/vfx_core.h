/*
 * vfx_core.h
 *
 *  Created on: 2019-07-03 20:05
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_CORE_H_
#define INC_USER_VFX_CORE_H_

#include <stdint.h>

typedef struct fifo_element {
    int16_t data;
    struct fifo_element *next;
} fifo_element_t;

extern uint32_t vfx_read_color_from_table(uint16_t color_idx, uint16_t color_ctr);
extern void vfx_write_pixel(uint8_t x, uint8_t y, uint8_t z, uint16_t color_idx, uint16_t color_ctr);
extern void vfx_fill_cube(uint8_t x, uint8_t y, uint8_t z, uint8_t cx, uint8_t cy, uint8_t cz, uint16_t color_idx, uint16_t color_ctr);
extern void vfx_write_cube_bitmap(const uint8_t *bitmap);
extern void vfx_write_layer_bitmap(uint8_t layer, const uint8_t *bitmap);
extern void vfx_write_layer_number(uint8_t num, uint8_t layer, uint16_t color_idx, uint16_t color_ctr);
extern void vfx_clear_cube(void);

extern void vfx_fifo_write(int16_t data);
extern int16_t vfx_fifo_read(void);
extern void vfx_fifo_init(void);

#endif /* INC_USER_VFX_CORE_H_ */
