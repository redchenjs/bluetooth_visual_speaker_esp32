/*
 * fft.c
 *
 *  Created on: 2021-01-14 17:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <complex.h>

#include "user/fft.h"

static float freq[FFT_N] = {0.0};

static   int bitrev[FFT_N] = {0.0};
static float window[FFT_N * 2] = {0.0};
static float complex root[FFT_N] = {0.0};
static float complex data[FFT_N] = {0.0};

static bool generated = false;

static int bit_reverse(int x)
{
    int y = 0;

    for (int i = FFT_N >> 1; i > 0; i >>= 1) {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }

    return y;
}

static void compute_fft_tables(void)
{
    // bit reversal
    for (int i = 0; i < FFT_N; i++) {
        bitrev[i] = bit_reverse(i);
    }

    // hamming window
    for (int i = 0; i < FFT_N * 2; i++) {
        window[i] = 0.53836 - 0.46164 * cosf(i * TWO_PI / (FFT_N * 2 - 1));
    }

    // twiddle factor
    for (int i = 0; i < FFT_N; i++) {
        root[i] = cexpf(-I * i * TWO_PI / (FFT_N * 2));
    }
}

static float compute_freq_lin(const float *freq, uint16_t step, uint16_t idx)
{
    float n = 0.0;

    for (int i = 0; i < step; i++) {
        n += freq[step * idx + i];
    }

    return n / step * 2.0;
}

static float compute_freq_band(const float *freq, const float *xscale, uint16_t bands, uint16_t band)
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

        if (b < FFT_N) {
            n += freq[b] * (xscale[band + 1] - b);
        }
    }

    return 20 * log10f(n * bands / FFT_N / 12.0);
}

void fft_compute_lin(uint16_t *data_out, uint16_t num, uint16_t step, uint16_t max_val, uint16_t min_val)
{
    for (int i = 0; i < num; i++) {
        data_out[i] += compute_freq_lin(freq, step, i) * (max_val / 40.0);
        data_out[i] /= 2.0;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_compute_log(uint16_t *data_out, uint16_t num, uint16_t step, uint16_t max_val, uint16_t min_val)
{
    for (int i = 0; i < num; i++) {
        data_out[i] += 20 * log10f(1 + compute_freq_lin(freq, step, i)) * (max_val / 40.0);
        data_out[i] /= 2.0;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_compute_bands(uint16_t *data_out, const float *xscale, uint16_t bands, uint16_t *delay, uint16_t max_val, uint16_t min_val)
{
    enum { BAND_FADE = 2, BAND_DELAY = 2 };

    for (int i = 0; i < bands; i++) {
        float x = (40 + compute_freq_band(freq, xscale, bands, i)) * (max_val / 64.0);

        data_out[i] = MAX(0, data_out[i] - (BAND_FADE - delay[i]));

        if (delay[i]) {
            delay[i]--;
        }

        if (x > data_out[i]) {
            data_out[i] = x;

            delay[i] = BAND_DELAY;
        }

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_compute_xscale(float *xscale, uint16_t bands)
{
    for (int i = 0; i <= bands; i++) {
        xscale[i] = powf(FFT_N, (float)i / bands) - 0.5;
    }
}

void fft_execute(float scale_factor)
{
    float complex even = 0, odd = 0;

    // Cooley–Tukey algorithm, radix-2 case
    int half = 1;
    for (int i = FFT_N >> 1; i > 0; i >>= 1) {
        for (int g = 0; g < FFT_N; g += half << 1) {
            for (int b = 0, r = 0; b < half; b++, r += i) {
                even = data[g + b];
                odd  = data[g + b + half] * root[r * 2];

                data[g + b]        = even + odd;
                data[g + b + half] = even - odd;
            }
        }

        half <<= 1;
    }

    // compute the amplitude of each frequency
    for (int i = 1; i < FFT_N; i++) {
        even = data[i] + conjf(data[FFT_N - i]);
        odd  = data[i] - conjf(data[FFT_N - i]);

        freq[i - 1] = 0.5 * cabsf(even - I * odd * root[i]) / FFT_N * scale_factor;
    }

    even = data[0] + conjf(data[0]);
    odd  = data[0] - conjf(data[0]);

    freq[FFT_N - 1] = 0.5 * cabsf(even + I * odd) / (FFT_N * 2) * scale_factor;
}

void fft_load_data(const uint8_t *data_in, fft_channel_t channel)
{
    switch (channel) {
        case FFT_CHANNEL_L:
            for (int i = 0; i < FFT_N; i++) {
                int16_t data_re_l = data_in[i * 8 + 1] << 8 | data_in[i * 8];
                int16_t data_im_l = data_in[i * 8 + 5] << 8 | data_in[i * 8 + 4];
                float data_re = data_re_l * window[i * 2];
                float data_im = data_im_l * window[i * 2 + 1];

                data[bitrev[i]] = data_re + I * data_im;
            }
            break;
        case FFT_CHANNEL_R:
            for (int i = 0; i < FFT_N; i++) {
                int16_t data_re_r = data_in[i * 8 + 3] << 8 | data_in[i * 8 + 2];
                int16_t data_im_r = data_in[i * 8 + 7] << 8 | data_in[i * 8 + 6];
                float data_re = data_re_r * window[i * 2];
                float data_im = data_im_r * window[i * 2 + 1];

                data[bitrev[i]] = data_re + I * data_im;
            }
            break;
        case FFT_CHANNEL_LR:
            for (int i = 0; i < FFT_N; i++) {
                int16_t data_re_l = data_in[i * 8 + 1] << 8 | data_in[i * 8];
                int16_t data_re_r = data_in[i * 8 + 3] << 8 | data_in[i * 8 + 2];
                int16_t data_im_l = data_in[i * 8 + 5] << 8 | data_in[i * 8 + 4];
                int16_t data_im_r = data_in[i * 8 + 7] << 8 | data_in[i * 8 + 6];
                float data_re = (data_re_l + data_re_r) / 2.0 * window[i * 2];
                float data_im = (data_im_l + data_im_r) / 2.0 * window[i * 2 + 1];

                data[bitrev[i]] = data_re + I * data_im;
            }
            break;
        default:
            break;
    }
}

void fft_init(void)
{
    memset(data, 0x00, sizeof(data));
    memset(freq, 0x00, sizeof(freq));

    if (!generated) {
        compute_fft_tables();

        generated = true;
    }
}
