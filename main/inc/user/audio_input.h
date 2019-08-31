/*
 * audio_input.h
 *
 *  Created on: 2019-07-05 21:22
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_INPUT_H_
#define INC_USER_AUDIO_INPUT_H_

#include <stdint.h>

extern void audio_input_set_mode(uint8_t mode);
extern uint8_t audio_input_get_mode(void);

extern void audio_input_init(void);

#endif /* INC_USER_AUDIO_INPUT_H_ */
