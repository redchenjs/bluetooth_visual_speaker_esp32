/*
 * gui_task.h
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_TASKS_GUI_TASK_H_
#define INC_TASKS_GUI_TASK_H_

#include <stdint.h>

// ani0.gif
extern const uint8_t ani0_gif_ptr[] asm("_binary_ani0_gif_start");
extern const uint8_t ani0_gif_end[] asm("_binary_ani0_gif_end");
// ani1.gif
extern const uint8_t ani1_gif_ptr[] asm("_binary_ani1_gif_start");
extern const uint8_t ani1_gif_end[] asm("_binary_ani1_gif_end");
// ani2.gif
extern const uint8_t ani2_gif_ptr[] asm("_binary_ani2_gif_start");
extern const uint8_t ani2_gif_end[] asm("_binary_ani2_gif_end");
// ani3.gif
extern const uint8_t ani3_gif_ptr[] asm("_binary_ani3_gif_start");
extern const uint8_t ani3_gif_end[] asm("_binary_ani3_gif_end");

extern void gui_show_image(uint8_t filename_index);
extern void gui_task(void *pvParameter);

#endif /* INC_TASKS_GUI_TASK_H_ */
