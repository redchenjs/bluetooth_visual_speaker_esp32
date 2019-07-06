/*
 * audio_render.h
 *
 *  Created on: 2018-04-05 16:41
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_AUDIO_RENDER_H_
#define INC_USER_AUDIO_RENDER_H_

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"

extern void render_sample_block_mono(short *short_sample_buff, int no_samples);
extern void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels);
extern void set_dac_sample_rate(int rate);

#endif /* INC_USER_AUDIO_RENDER_H_ */
