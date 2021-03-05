/*
 * fft.h
 *
 *  Created on: 2021-01-14 17:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_USER_FFT_H_
#define INC_USER_FFT_H_

#include <stdint.h>

#define TWO_PI         (6.2831853f)

#define FFT_N          (256)
#define FFT_BLOCK_SIZE (FFT_N * 8)

#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define MAX(a, b)      ((a) > (b) ? (a) : (b))

typedef enum {
    FFT_CHANNEL_L  = 0x00,
    FFT_CHANNEL_R  = 0x01,
    FFT_CHANNEL_LR = 0x02
} fft_channel_t;

extern void fft_compute_lin(uint16_t *data_out, uint16_t num, uint16_t step, uint16_t max_val, uint16_t min_val);
extern void fft_compute_log(uint16_t *data_out, uint16_t num, uint16_t step, uint16_t max_val, uint16_t min_val);

extern void fft_compute_bands(uint16_t *data_out, const float *xscale, uint16_t bands, uint16_t *delay, uint16_t max_val, uint16_t min_val);
extern void fft_compute_xscale(float *xscale, uint16_t bands);

extern void fft_execute(float scale_factor);
extern void fft_load_data(const uint8_t *data_in, fft_channel_t channel);

extern void fft_init(void);

#endif /* INC_USER_FFT_H_ */
