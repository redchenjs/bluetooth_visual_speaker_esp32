/*
 * audio_mp3.h
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_MP3_H_
#define INC_USER_AUDIO_MP3_H_

#include <stdint.h>

// snd0.mp3
extern const char snd0_mp3_ptr[] asm("_binary_snd0_mp3_start");
extern const char snd0_mp3_end[] asm("_binary_snd0_mp3_end");
// snd1.mp3
extern const char snd1_mp3_ptr[] asm("_binary_snd1_mp3_start");
extern const char snd1_mp3_end[] asm("_binary_snd1_mp3_end");
// snd2.mp3
extern const char snd2_mp3_ptr[] asm("_binary_snd2_mp3_start");
extern const char snd2_mp3_end[] asm("_binary_snd2_mp3_end");
// snd3.mp3
extern const char snd3_mp3_ptr[] asm("_binary_snd3_mp3_start");
extern const char snd3_mp3_end[] asm("_binary_snd3_mp3_end");

extern void audio_mp3_play_file(uint8_t idx);

extern void audio_mp3_init(void);

#endif /* INC_USER_AUDIO_MP3_H_ */
