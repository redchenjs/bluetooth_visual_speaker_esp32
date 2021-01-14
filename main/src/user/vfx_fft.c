/*
 * vfx_fft.c
 *
 *  Created on: 2021-01-14 17:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <math.h>
#include <string.h>

#include "esp_dsp.h"

#include "user/vfx.h"
#include "user/vfx_fft.h"

float vfx_fft_wind[FFT_N] = {0.0};
float vfx_fft_data[FFT_N] = {0.0};

void vfx_fft_compute_lin(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += sqrt(data_in[0] * data_in[0] + data_in[1] * data_in[1]) / FFT_N * (max_val * scale_factor / 32768.0);
    data_out[0] /= 2;
    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (uint16_t k = 1; k < FFT_N / 2; k++) {
        data_out[k] += sqrt(data_in[2 * k] * data_in[2 * k] + data_in[2 * k + 1] * data_in[2 * k + 1]) / FFT_N * 2 * (max_val * scale_factor / 32768.0);
        data_out[k] /= 2;
        if (data_out[k] > max_val) {
            data_out[k] = max_val;
        } else if (data_out[k] < min_val) {
            data_out[k] = min_val;
        }
    }
}

void vfx_fft_compute_log(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += 20 * log10(1 + sqrt(data_in[0] * data_in[0] + data_in[1] * data_in[1]) / FFT_N) * (max_val * scale_factor / 32768.0);
    data_out[0] /= 2;
    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (uint16_t k = 1; k < FFT_N / 2; k++) {
        data_out[k] += 20 * log10(1 + sqrt(data_in[2 * k] * data_in[2 * k] + data_in[2 * k + 1] * data_in[2 * k + 1]) / FFT_N * 2) * (max_val * scale_factor / 32768.0);
        data_out[k] /= 2;
        if (data_out[k] > max_val) {
            data_out[k] = max_val;
        } else if (data_out[k] < min_val) {
            data_out[k] = min_val;
        }
    }
}

void vfx_fft_execute(void)
{
    for (int i = 0; i < FFT_N; i++) {
        vfx_fft_data[i] *= vfx_fft_wind[i];
    }

    dsps_fft4r_fc32(vfx_fft_data, FFT_N >> 1);
    dsps_bit_rev4r_fc32(vfx_fft_data, FFT_N >> 1);
}

void vfx_fft_init(void)
{
    memset(vfx_fft_data, 0x00, sizeof(vfx_fft_data));

    dsps_fft4r_init_fc32(NULL, FFT_N >> 1);
    dsps_wind_hann_f32(vfx_fft_wind, FFT_N);
}

void vfx_fft_deinit(void)
{
    dsps_fft2r_deinit_fc32();
}
