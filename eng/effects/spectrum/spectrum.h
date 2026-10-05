#ifndef SPECTRUM_H
#define SPECTRUM_H


#define SPECTRUM_FFT_SIZE  2048
#define SPECTRUM_MAX_BANDS 128

void spectrum_init(int sample_rate);
void spectrum_push(const float *buf, int n_frames, int channels);
int spectrum_get_bands(float *bands, float *peaks, int n_bands);

#endif