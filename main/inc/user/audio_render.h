/*
 * audio_render.h
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_RENDER_H_
#define INC_USER_AUDIO_RENDER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

extern RingbufHandle_t audio_buff;

extern void audio_render_init(void);

#endif /* INC_USER_AUDIO_RENDER_H_ */
