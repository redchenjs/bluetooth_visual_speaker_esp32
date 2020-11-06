/*
 * ain.h
 *
 *  Created on: 2019-07-05 21:22
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_INPUT_H_
#define INC_USER_AUDIO_INPUT_H_

typedef enum {
    AIN_MODE_IDX_OFF = 0x00,
    AIN_MODE_IDX_ON  = 0x01
} ain_mode_t;

#define DEFAULT_AIN_MODE AIN_MODE_IDX_ON

extern void ain_set_mode(ain_mode_t idx);
extern ain_mode_t ain_get_mode(void);

extern void ain_init(void);

#endif /* INC_USER_AUDIO_INPUT_H_ */
