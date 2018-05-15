/*
 * led_daemon.h
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_LED_DAEMON_H_
#define INC_USER_LED_DAEMON_H_

#include <stdint.h>

extern void led_daemon(void *pvParameter);
extern void led_set_mode(uint8_t mode);

#endif /* INC_USER_LED_DAEMON_H_ */
