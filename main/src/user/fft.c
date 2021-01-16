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

static   int bitrev[FFT_N] = {0.0};
static float window[FFT_N] = {0.0};
static float complex data[FFT_N] = {0.0};
static float complex root[FFT_N / 2] = {0.0};

static  bool generated = false;
static  char xdelay[BAND_N] = {0};
static float xscale[BAND_N + 1] = {0.0};

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
    for (int i = 0; i < FFT_N; i++) {
        bitrev[i] = bit_reverse(i);
    }

    // Hanning window
    for (int i = 0; i < FFT_N; i++) {
        window[i] = 0.5 * (1 - cosf(i * (TWO_PI / FFT_N)));
    }

    for (int i = 0; i < FFT_N / 2; i++) {
        root[i] = cexpf(-i * (TWO_PI / FFT_N) * I);
    }
}

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

void fft_compute_lin(uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += cabsf(data[0]) / FFT_N * (max_val * scale_factor / 16384.0);
    data_out[0] /= 2;

    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (int i = 1; i < FFT_OUT_N; i++) {
        data_out[i] += cabsf(data[(FFT_N / 2 / FFT_OUT_N) * i]) / FFT_N * 2 * (max_val * scale_factor / 16384.0);
        data_out[i] /= 2;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_compute_log(uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    data_out[0] += 20 * log10f(1 + cabsf(data[0]) / FFT_N) * (max_val * scale_factor / 16384.0);
    data_out[0] /= 2;

    if (data_out[0] > max_val) {
        data_out[0] = max_val;
    } else if (data_out[0] < min_val) {
        data_out[0] = min_val;
    }

    for (int i = 1; i < FFT_OUT_N; i++) {
        data_out[i] += 20 * log10f(1 + cabsf(data[(FFT_N / 2 / FFT_OUT_N) * i]) / FFT_N * 2) * (max_val * scale_factor / 16384.0);
        data_out[i] /= 2;

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_compute_bands(uint16_t *data_out, uint16_t scale_factor, uint16_t max_val, uint16_t min_val)
{
    float freq[FFT_N / 2] = {0.0};

    freq[0] = cabsf(data[0]) / FFT_N * (scale_factor / 2.0 / 65536.0);

    for (int i = 1; i < FFT_N / 2; i++) {
        freq[i] = 2 * cabsf(data[i]) / FFT_N * (scale_factor / 2.0 / 65536.0);
    }

    for (int i = 0; i < BAND_N; i++) {
        float x = (40 + compute_freq_band(freq, xscale, i, BAND_N)) * (max_val / 64.0);

        data_out[i] = MAX(0, data_out[i] - (BAND_FADE - xdelay[i]));

        if (xdelay[i]) {
            xdelay[i]--;
        }

        if (x > data_out[i]) {
            data_out[i]= x;

            xdelay[i] = BAND_DELAY;
        }

        if (data_out[i] > max_val) {
            data_out[i] = max_val;
        } else if (data_out[i] < min_val) {
            data_out[i] = min_val;
        }
    }
}

void fft_execute(void)
{
    int half = 1;

    // Cooley–Tukey Fast Fourier Transform, radix-2 case
    for (int i = FFT_N >> 1; i > 0; i >>= 1) {
        for (int g = 0; g < FFT_N; g += half << 1) {
            for (int b = 0, r = 0; b < half; b++, r += i) {
                float complex even = data[g + b];
                float complex odd  = data[g + b + half] * root[r];

                data[g + b] = even + odd;
                data[g + b + half] = even - odd;
            }
        }

        half <<= 1;
    }
}

void fft_load_data(const uint8_t *data_in, fft_channel_t channel)
{
    int16_t data_l = 0, data_r = 0;

    switch (channel) {
        case FFT_CHANNEL_L:
            for (int i = 0; i < FFT_N; i++) {
                data_l = data_in[i * 4 + 1] << 8 | data_in[i * 4];

                data[bitrev[i]] = (float)data_l * window[i];
            }
            break;
        case FFT_CHANNEL_R:
            for (int i = 0; i < FFT_N; i++) {
                data_r = data_in[i * 4 + 3] << 8 | data_in[i * 4 + 2];

                data[bitrev[i]] = (float)data_r * window[i];
            }
            break;
        case FFT_CHANNEL_LR:
            for (int i = 0; i < FFT_N; i++) {
                data_l = data_in[i * 4 + 1] << 8 | data_in[i * 4];
                data_r = data_in[i * 4 + 3] << 8 | data_in[i * 4 + 2];

                data[bitrev[i]] = (float)((data_l + data_r) / 2) * window[i];
            }
            break;
        default:
            break;
    }
}

void fft_init(void)
{
    memset(data, 0x00, sizeof(data));

    if (!generated) {
        compute_fft_tables();
        compute_log_xscale(xscale, BAND_N);

        generated = true;
    }
}
