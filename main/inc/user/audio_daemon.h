/*
 * audio_daemon.h
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_DAEMON_H_
#define INC_USER_AUDIO_DAEMON_H_

#include <stdint.h>

// snd0.mp3
extern const uint8_t snd0_mp3_ptr[] asm("_binary_snd0_mp3_start");
extern const uint8_t snd0_mp3_end[] asm("_binary_snd0_mp3_end");
// snd1.mp3
extern const uint8_t snd1_mp3_ptr[] asm("_binary_snd1_mp3_start");
extern const uint8_t snd1_mp3_end[] asm("_binary_snd1_mp3_end");

extern void audio_daemon(void *pvParameter);
extern void audio_play_file(uint8_t filename_index);

#endif /* INC_USER_AUDIO_DAEMON_H_ */
