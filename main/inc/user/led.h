/*
 * led.h
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_LED_H_
#define INC_USER_LED_H_

#include <stdint.h>

extern void led_set_mode(uint8_t idx);

extern void led_init(void);

#endif /* INC_USER_LED_H_ */
