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

float vfx_fft_data[FFT_N] = {0.0};

static float window[FFT_N] = {0.0};
static float xscale[FFT_BANDS_N + 1] = {0.0};

static void compute_log_xscale(float *xscale, int bands)
{
    for (int i = 0; i <= bands; i++) {
        xscale[i] = powf(FFT_N / 2, (float)i / bands) - 0.5f;
    }
}

static float compute_freq_band(const float *freq, const float *xscale, int band, int bands)
{
    float n = 0.0;
    int a = ceilf(xscale[band]);
    int b = floorf(xscale[band + 1]);

    if (b < a) {
        n += freq[b] * (xscale[band + 1] - xscale[band]);
    } else {
        if (a > 0) {
            n += freq[a - 1] * (a - xscale[band]);
        }
        for (; a < b; a++) {
            n += freq[a];
        }
        if (b < FFT_N / 2) {
            n += freq[b] * (xscale[band + 1] - b);
        }
    }

    n *= (float)bands / 12;

    return 20 * log10f(n);
}

void vfx_fft_compute_bands(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    float freq[FFT_N / 2] = {0.0};

    for (int n = 0; n < FFT_N / 2 - 1; n++) {
        freq[n] = 2 * fabs(data_in[1 + n]) / FFT_N * (scale_factor / 8.0 / 16384.0);
    }

    freq[FFT_N / 2 - 1] = fabs(data_in[FFT_N / 2]) / FFT_N * (scale_factor / 8.0 / 16384.0);

    for (int i = 0; i < FFT_BANDS_N; i++) {
        float x = (40 + compute_freq_band(freq, xscale, i, FFT_BANDS_N)) * (max_val / 64.0);

        if (x > data_out[i]) {
            data_out[i] = x;
        } else if (data_out[i] > 0) {
            data_out[i] *= 0.95;
        }

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void vfx_fft_compute_lin(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += sqrtf(data_in[0] * data_in[0] + data_in[1] * data_in[1]) / FFT_N * (max_val * scale_factor / 16384.0);
    data_out[0] /= 2;

    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (int i = 1; i < 64; i++) {
        data_out[i] += sqrtf(data_in[FFT_BLOCK_STEP * i] * data_in[FFT_BLOCK_STEP * i] + data_in[FFT_BLOCK_STEP * i + 1] * data_in[FFT_BLOCK_STEP * i + 1]) / FFT_N * 2 * (max_val * scale_factor / 16384.0);
        data_out[i] /= 2;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void vfx_fft_compute_log(const float *data_in, uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += 20 * log10f(1 + sqrtf(data_in[0] * data_in[0] + data_in[1] * data_in[1]) / FFT_N) * (max_val * scale_factor / 16384.0);
    data_out[0] /= 2;

    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (int i = 1; i < 64; i++) {
        data_out[i] += 20 * log10f(1 + sqrtf(data_in[FFT_BLOCK_STEP * i] * data_in[FFT_BLOCK_STEP * i] + data_in[FFT_BLOCK_STEP * i + 1] * data_in[FFT_BLOCK_STEP * i + 1]) / FFT_N * 2) * (max_val * scale_factor / 16384.0);
        data_out[i] /= 2;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void vfx_fft_execute(void)
{
    for (int i = 0; i < FFT_N; i++) {
        vfx_fft_data[i] *= window[i];
    }

    dsps_fft4r_fc32(vfx_fft_data, FFT_N >> 1);
    dsps_bit_rev4r_fc32(vfx_fft_data, FFT_N >> 1);
}

void vfx_fft_init(void)
{
    memset(vfx_fft_data, 0x00, sizeof(vfx_fft_data));

    dsps_fft4r_init_fc32(NULL, FFT_N >> 1);
    dsps_wind_hann_f32(window, FFT_N);

    compute_log_xscale(xscale, FFT_BANDS_N);
}

void vfx_fft_deinit(void)
{
    dsps_fft2r_deinit_fc32();
}
