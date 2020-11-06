/*
 * led.h
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_LED_H_
#define INC_USER_LED_H_

typedef enum {
    LED_MODE_IDX_BLINK_S1 = 0x00,
    LED_MODE_IDX_BLINK_S0 = 0x01,
    LED_MODE_IDX_BLINK_M1 = 0x02,
    LED_MODE_IDX_BLINK_M0 = 0x03,
    LED_MODE_IDX_BLINK_F1 = 0x04,
    LED_MODE_IDX_BLINK_F0 = 0x05,
    LED_MODE_IDX_PULSE_D0 = 0x06,
    LED_MODE_IDX_PULSE_D1 = 0x07,
    LED_MODE_IDX_PULSE_D2 = 0x08,
    LED_MODE_IDX_PULSE_D3 = 0x09
} led_mode_t;

extern void led_set_mode(led_mode_t idx);
extern led_mode_t led_get_mode(void);

extern void led_init(void);

#endif /* INC_USER_LED_H_ */
