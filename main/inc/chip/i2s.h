/*
 * i2s.h
 *
 *  Created on: 2018-02-10 16:37
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CHIP_I2S_H_
#define INC_CHIP_I2S_H_

extern void i2s0_init(void);
extern void i2s1_init(void);

extern void i2s_set_output_sample_rate(int rate);
extern void i2s_set_input_sample_rate(int rate);

#endif /* INC_CHIP_I2S_H_ */
