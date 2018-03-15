/*
 * gui_task.h
 *
 *  Created on: 2018-02-13 22:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_TASKS_GUI_TASK_H_
#define INC_TASKS_GUI_TASK_H_

#include <stdint.h>

#if defined(CONFIG_OLED_PANEL_SSD1331)
// ani0_96x64.gif
extern const uint8_t ani0_96x64_gif_ptr[] asm("_binary_ani0_96x64_gif_start");
extern const uint8_t ani0_96x64_gif_end[] asm("_binary_ani0_96x64_gif_end");
// ani1_96x64.gif
extern const uint8_t ani1_96x64_gif_ptr[] asm("_binary_ani1_96x64_gif_start");
extern const uint8_t ani1_96x64_gif_end[] asm("_binary_ani1_96x64_gif_end");
// ani2_96x64.gif
extern const uint8_t ani2_96x64_gif_ptr[] asm("_binary_ani2_96x64_gif_start");
extern const uint8_t ani2_96x64_gif_end[] asm("_binary_ani2_96x64_gif_end");
// ani3_96x64.gif
extern const uint8_t ani3_96x64_gif_ptr[] asm("_binary_ani3_96x64_gif_start");
extern const uint8_t ani3_96x64_gif_end[] asm("_binary_ani3_96x64_gif_end");
#elif defined(CONFIG_OLED_PANEL_SSD1351)
// ani0_128x128.gif
extern const uint8_t ani0_128x128_gif_ptr[] asm("_binary_ani0_128x128_gif_start");
extern const uint8_t ani0_128x128_gif_end[] asm("_binary_ani0_128x128_gif_end");
// ani1_128x128.gif
extern const uint8_t ani1_128x128_gif_ptr[] asm("_binary_ani1_128x128_gif_start");
extern const uint8_t ani1_128x128_gif_end[] asm("_binary_ani1_128x128_gif_end");
// ani2_128x128.gif
extern const uint8_t ani2_128x128_gif_ptr[] asm("_binary_ani2_128x128_gif_start");
extern const uint8_t ani2_128x128_gif_end[] asm("_binary_ani2_128x128_gif_end");
// ani3_128x128.gif
extern const uint8_t ani3_128x128_gif_ptr[] asm("_binary_ani3_128x128_gif_start");
extern const uint8_t ani3_128x128_gif_end[] asm("_binary_ani3_128x128_gif_end");
#endif

extern void gui_show_image(uint8_t filename_index);
extern void gui_task(void *pvParameter);

#endif /* INC_TASKS_GUI_TASK_H_ */
