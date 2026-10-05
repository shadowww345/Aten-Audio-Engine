#define _POSIX_C_SOURCE 200809L
#include "spectrum.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>

#define SPEC_PI    3.14159265358979323846f
#define N          SPECTRUM_FFT_SIZE
#define RING_SIZE  16384
#define RING_MASK  (RING_SIZE - 1)

#define FREQ_MIN        40.0f 
#define FREQ_MAX        16000.0f
#define DB_FLOOR        (-70.0f)
#define DB_CEIL         (-10.0f)
#define TILT_DB_OCT     3.0f
#define ATTACK_TAU      0.025f
#define DECAY_TAU       0.20f
#define PEAK_FALL       0.50f

static float        s_ring[RING_SIZE];
static atomic_uint  s_write;

static int      s_inited = 0;
static int      s_rate   = 44100;
static float    s_window[N];
static float    s_cos[N / 2], s_sin[N / 2];
static uint16_t s_rev[N];
static float    s_mag[N / 2];
static float    s_bands[SPECTRUM_MAX_BANDS];
static float    s_peaks[SPECTRUM_MAX_BANDS];
static double   s_last_time = 0.0;

static inline float clamp01(float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void spectrum_init(int sample_rate)
{
    s_rate = sample_rate > 0 ? sample_rate : 44100;

    for (int i = 0; i < N; i++)
        s_window[i] = 0.5f * (1.0f - cosf(2.0f * SPEC_PI * (float)i / (float)N)); /* Hann */

    for (int k = 0; k < N / 2; k++) {
        float a = 2.0f * SPEC_PI * (float)k / (float)N;
        s_cos[k] = cosf(a);
        s_sin[k] = sinf(a);
    }

    int bits = 0;
    while ((1 << bits) < N)
        bits++;
    for (int i = 0; i < N; i++) {
        int r = 0;
        for (int b = 0; b < bits; b++)
            if (i & (1 << b))
                r |= 1 << (bits - 1 - b);
        s_rev[i] = (uint16_t)r;
    }

    memset(s_bands, 0, sizeof(s_bands));
    memset(s_peaks, 0, sizeof(s_peaks));
    s_last_time = 0.0;
    s_inited = 1;
}

void spectrum_push(const float *buf, int n_frames, int channels)
{
    if (channels < 1)
        return;
    unsigned wp = atomic_load_explicit(&s_write, memory_order_relaxed);
    const float inv = 1.0f / (float)channels;

    for (int i = 0; i < n_frames; i++) {
        float s = 0.0f;
        for (int c = 0; c < channels; c++)
            s += buf[i * channels + c];
        s_ring[wp & RING_MASK] = s * inv;
        wp++;
    }
    atomic_store_explicit(&s_write, wp, memory_order_release);
}

static void fft(float *re, float *im)
{
    for (int i = 0; i < N; i++) {
        int j = s_rev[i];
        if (j > i) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= N; len <<= 1) {
        int half = len >> 1;
        int step = N / len;
        for (int i = 0; i < N; i += len) {
            for (int k = 0; k < half; k++) {
                float wr = s_cos[k * step];
                float wi = -s_sin[k * step];
                int a = i + k, b = i + k + half;
                float tr = re[b] * wr - im[b] * wi;
                float ti = re[b] * wi + im[b] * wr;
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
            }
        }
    }
}

int spectrum_get_bands(float *bands, float *peaks, int n_bands)
{
    if (!bands || n_bands <= 0)
        return -1;
    if (n_bands > SPECTRUM_MAX_BANDS)
        n_bands = SPECTRUM_MAX_BANDS;
    if (!s_inited)
        spectrum_init(44100);

    float re[N], im[N];
    unsigned wp = atomic_load_explicit(&s_write, memory_order_acquire);
    unsigned start = wp - (unsigned)N;
    for (int i = 0; i < N; i++) {
        re[i] = s_ring[(start + (unsigned)i) & RING_MASK] * s_window[i];
        im[i] = 0.0f;
    }
    fft(re, im);

    const float norm = 4.0f / (float)N;
    for (int k = 0; k < N / 2; k++)
        s_mag[k] = sqrtf(re[k] * re[k] + im[k] * im[k]) * norm;

    double t = now_sec();
    float dt = (s_last_time > 0.0) ? (float)(t - s_last_time) : (1.0f / 60.0f);
    s_last_time = t;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.100f) dt = 0.100f;
    const float a_att = 1.0f - expf(-dt / ATTACK_TAU);
    const float a_dec = 1.0f - expf(-dt / DECAY_TAU);

    /* --- log-spaced bands --- */
    const float binhz = (float)s_rate / (float)N;
    float fmax = FREQ_MAX;
    float nyq = 0.98f * (float)s_rate * 0.5f;
    if (fmax > nyq)
        fmax = nyq;
    const float ratio = fmax / FREQ_MIN;

    for (int b = 0; b < n_bands; b++) {
        float f0 = FREQ_MIN * powf(ratio, (float)b / (float)n_bands);
        float f1 = FREQ_MIN * powf(ratio, (float)(b + 1) / (float)n_bands);
        float fc = sqrtf(f0 * f1);
        float x0 = f0 / binhz, x1 = f1 / binhz;
        float m = 0.0f;

        if (x1 - x0 < 1.0f) {
            float x = fc / binhz;
            int k = (int)x;
            float fr = x - (float)k;
            if (k > N / 2 - 2) { k = N / 2 - 2; fr = 1.0f; }
            m = s_mag[k] * (1.0f - fr) + s_mag[k + 1] * fr;
        } else {
            int k0 = (int)ceilf(x0), k1 = (int)floorf(x1);
            if (k0 < 1) k0 = 1;
            if (k1 > N / 2 - 1) k1 = N / 2 - 1;
            if (k0 > k1) k0 = k1;
            for (int k = k0; k <= k1; k++)
                if (s_mag[k] > m)
                    m = s_mag[k];
        }

        float db = 20.0f * log10f(m + 1e-9f) + TILT_DB_OCT * log2f(fc / 1000.0f);
        float v = clamp01((db - DB_FLOOR) / (DB_CEIL - DB_FLOOR));

        s_bands[b] += (v - s_bands[b]) * (v > s_bands[b] ? a_att : a_dec);

        s_peaks[b] -= PEAK_FALL * dt;
        if (s_bands[b] > s_peaks[b])
            s_peaks[b] = s_bands[b];
        if (s_peaks[b] < 0.0f)
            s_peaks[b] = 0.0f;

        bands[b] = s_bands[b];
        if (peaks)
            peaks[b] = s_peaks[b];
    }
    return n_bands;
}