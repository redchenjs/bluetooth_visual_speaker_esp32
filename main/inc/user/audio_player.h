/*
 * audio_player.h
 *
 *  Created on: 2018-02-12 20:13
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_PLAYER_H_
#define INC_USER_AUDIO_PLAYER_H_

typedef enum {
    MP3_FILE_IDX_CONNECTED    = 0x00,
    MP3_FILE_IDX_DISCONNECTED = 0x01,
    MP3_FILE_IDX_SLEEP        = 0x02,
    MP3_FILE_IDX_WAKEUP       = 0x03,

    MP3_FILE_IDX_MAX
} mp3_file_t;

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

extern void audio_player_play_file(mp3_file_t idx);

extern void audio_player_init(void);

#endif /* INC_USER_AUDIO_PLAYER_H_ */
