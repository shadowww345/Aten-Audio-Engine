#include "echo.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>

#define ECHO_SLEW 0.0015f
#define ECHO_SILENCE 1e-4f

static _Atomic float p_delay_ms = 350.0f;
static _Atomic float p_feedback = 0.40f;
static _Atomic float p_wet      = 0.50f;
static _Atomic float p_damp     = 0.30f;
static atomic_int    p_pingpong = 0;

static float *g_buf[2] = { NULL, NULL };
static int    g_size   = 0;
static int    g_pos    = 0;
static int    g_rate   = 44100;
static float  g_lp[2]  = { 0.0f, 0.0f };
static float  g_delay_cur = 0.0f;/
static int    g_tail   = 0;

static inline float clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

int echo_init(int sample_rate)
{
    if (g_buf[0])
        return 0;
    if (sample_rate <= 0)
        sample_rate = 44100;

    g_rate = sample_rate;
    g_size = (int)(ECHO_MAX_DELAY_MS * 0.001f * (float)sample_rate) + 4;

    g_buf[0] = calloc((size_t)g_size, sizeof(float));
    g_buf[1] = calloc((size_t)g_size, sizeof(float));
    if (!g_buf[0] || !g_buf[1]) {
        echo_free();
        return -1;
    }

    g_pos = 0;
    g_lp[0] = g_lp[1] = 0.0f;
    g_tail = 0;
    g_delay_cur = atomic_load(&p_delay_ms) * 0.001f * (float)g_rate;
    return 0;
}

void echo_free(void)
{
    free(g_buf[0]); g_buf[0] = NULL;
    free(g_buf[1]); g_buf[1] = NULL;
    g_size = 0;
    g_pos = 0;
    g_tail = 0;
}

void echo_reset(void)
{
    if (!g_buf[0])
        return;
    memset(g_buf[0], 0, (size_t)g_size * sizeof(float));
    memset(g_buf[1], 0, (size_t)g_size * sizeof(float));
    g_lp[0] = g_lp[1] = 0.0f;
    g_tail = 0;
}

void echo_set_delay(float ms)
{
    atomic_store_explicit(&p_delay_ms, clampf(ms, 1.0f, ECHO_MAX_DELAY_MS),
                          memory_order_relaxed);
}

void echo_set_feedback(float feedback)
{
    atomic_store_explicit(&p_feedback, clampf(feedback, 0.0f, 0.95f),
                          memory_order_relaxed);
}

void echo_set_wet(float wet)
{
    atomic_store_explicit(&p_wet, clampf(wet, 0.0f, 4.0f),
                          memory_order_relaxed);
}

void echo_set_damp(float damp)
{
    atomic_store_explicit(&p_damp, clampf(damp, 0.0f, 1.0f),
                          memory_order_relaxed);
}

void echo_set_pingpong(int enabled)
{
    atomic_store_explicit(&p_pingpong, enabled ? 1 : 0, memory_order_relaxed);
}

int echo_has_tail(void)
{
    return g_tail > 0;
}

void echo_process_replace(float *buf, int n_frames, int channels)
{
    if (!g_buf[0] || channels < 1 || channels > 2) {
        memset(buf, 0, (size_t)n_frames * (size_t)(channels > 0 ? channels : 1) * sizeof(float));
        return;
    }

    const float fb   = atomic_load_explicit(&p_feedback, memory_order_relaxed);
    const float wet  = atomic_load_explicit(&p_wet,      memory_order_relaxed);
    const float damp = atomic_load_explicit(&p_damp,     memory_order_relaxed);
    const int   pp   = (channels == 2) &&
                       atomic_load_explicit(&p_pingpong, memory_order_relaxed);
    const float ms   = atomic_load_explicit(&p_delay_ms, memory_order_relaxed);

    float target = ms * 0.001f * (float)g_rate;
    target = clampf(target, 1.0f, (float)(g_size - 3));

    const float k = clampf(1.0f - damp, 0.05f, 1.0f);

    float peak = 0.0f;

    for (int i = 0; i < n_frames; i++) {
        g_delay_cur += (target - g_delay_cur) * ECHO_SLEW;

        float rp = (float)g_pos - g_delay_cur;
        if (rp < 0.0f)
            rp += (float)g_size;
        int i0 = (int)rp;
        float fr = rp - (float)i0;
        int i1 = i0 + 1;
        if (i1 >= g_size)
            i1 = 0;

        float in[2] = { 0.0f, 0.0f };
        float r[2]  = { 0.0f, 0.0f };

        for (int c = 0; c < channels; c++) {
            in[c] = buf[i * channels + c];
            r[c]  = g_buf[c][i0] + (g_buf[c][i1] - g_buf[c][i0]) * fr;
            g_lp[c] += k * (r[c] - g_lp[c]);

            float a = fabsf(in[c]);
            if (a > peak)
                peak = a;
        }

        float w[2];
        if (pp) {
            w[0] = (in[0] + in[1]) * 0.5f + fb * g_lp[1];
            w[1] = fb * g_lp[0];
        } else {
            for (int c = 0; c < channels; c++)
                w[c] = in[c] + fb * g_lp[c];
        }

        for (int c = 0; c < channels; c++) {
            if (fabsf(w[c]) < 1e-20f)
                w[c] = 0.0f;
            g_buf[c][g_pos] = w[c];
            buf[i * channels + c] = r[c] * wet;
        }

        if (++g_pos >= g_size)
            g_pos = 0;
    }
    
    if (peak > ECHO_SILENCE) {
        float reps = (fb < 0.01f) ? 1.0f : 6.9f / -logf(fb);
        if (reps > 40.0f)
            reps = 40.0f;
        g_tail = (int)(target * (reps + 1.0f));
    } else if (g_tail > 0) {
        g_tail -= n_frames;
        if (g_tail < 0)
            g_tail = 0;
    }
}