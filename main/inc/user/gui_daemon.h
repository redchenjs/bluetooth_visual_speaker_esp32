/*
 * gui_daemon.h
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_GUI_DAEMON_H_
#define INC_USER_GUI_DAEMON_H_

#include <stdint.h>

extern void gui_daemon(void *pvParameter);
extern void gui_set_mode(uint8_t mode);
extern uint8_t gui_get_mode(void);

#endif /* INC_USER_GUI_DAEMON_H_ */
