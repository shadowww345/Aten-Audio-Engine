#include "resample.h"
#include "eng_pipewire.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int resample_voice(struct voice *v, uint32_t target_rate) {
    if (v == NULL || v->audio_data == NULL || v->channels == 0 || target_rate == 0)
        return -1;
    if (v->sample_rate == 0 || v->sample_rate == target_rate)
        return 0; 

    uint32_t src_channels = v->channels;
    uint32_t src_frames = v->data_size / ((uint32_t)sizeof(int16_t) * src_channels);
    if (src_frames == 0)
        return 0;

    double ratio        = (double)target_rate / (double)v->sample_rate;
    uint32_t dst_frames  = (uint32_t)((double)src_frames * ratio + 0.5);
    if (dst_frames == 0)
        dst_frames = 1;

    int16_t *src = (int16_t *)v->audio_data;
    int16_t *dst = (int16_t *)malloc((size_t)dst_frames * src_channels * sizeof(int16_t));
    if (dst == NULL) {
        printf("[ERROR]Resample: out of memory\n");
        return -1;
    }
    double step = (double)src_frames / (double)dst_frames;
    for (uint32_t i = 0; i < dst_frames; i++) {
        double pos  = (double)i * step;
        uint32_t i0 = (uint32_t)pos;
        uint32_t i1 = i0 + 1;
        if (i1 >= src_frames) i1 = src_frames - 1;
        double frac = pos - (double)i0;

        for (uint32_t c = 0; c < src_channels; c++) {
            int16_t s0 = src[i0 * src_channels + c];
            int16_t s1 = src[i1 * src_channels + c];
            dst[i * src_channels + c] = (int16_t)(s0 + ((double)(s1 - s0)) * frac);
        }
    }
    printf("%s","Voice resampled \n");
    free(v->audio_data);
    v->audio_data  = (uint8_t *)dst;
    v->data_size   = dst_frames * src_channels * (uint32_t)sizeof(int16_t);
    v->data_pos    = 0;
    v->sample_rate = target_rate;

    return 0;
}