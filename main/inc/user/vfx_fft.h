/*
 * vfx_fft.h
 *
 *  Created on: 2021-01-14 17:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_VFX_FFT_H_
#define INC_USER_VFX_FFT_H_

#include <stdint.h>

#define FFT_N          (128)
#define FFT_BLOCK_SIZE (FFT_N * 4)

extern float vfx_fft_data[FFT_N];

extern void vfx_fft_execute(void);

extern void vfx_fft_compute_lin(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val);
extern void vfx_fft_compute_log(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val);

extern void vfx_fft_init(void);
extern void vfx_fft_deinit(void);

#endif /* INC_USER_VFX_FFT_H_ */
