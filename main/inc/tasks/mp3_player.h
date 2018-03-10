/*
 * mp3_player.h
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_TASKS_MP3_PLAYER_H_
#define INC_TASKS_MP3_PLAYER_H_

#include <stdint.h>

// snd0.mp3
extern const uint8_t snd0_mp3_ptr[] asm("_binary_snd0_mp3_start");
extern const uint8_t snd0_mp3_end[] asm("_binary_snd0_mp3_end");
// snd1.mp3
extern const uint8_t snd1_mp3_ptr[] asm("_binary_snd1_mp3_start");
extern const uint8_t snd1_mp3_end[] asm("_binary_snd1_mp3_end");

extern void mp3_player_play_file(uint8_t filename_index);
extern void mp3_player_task(void *pvParameter);

extern void set_dac_sample_rate(int rate);

#endif /* INC_TASKS_MP3_PLAYER_H_ */
