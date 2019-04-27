/*
 * vfx.h
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_H_
#define INC_USER_VFX_H_

#include <stdint.h>

extern void vfx_task(void *pvParameter);
extern void vfx_set_mode(uint8_t mode);
extern uint8_t vfx_get_mode(void);

#endif /* INC_USER_VFX_H_ */
