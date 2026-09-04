#ifndef FREEVERB_H
#define FREEVERB_H


void reverb_init(void);

void reverb_set_roomsize(float value);
void reverb_set_damp(float value);
void reverb_set_wet(float value);
void reverb_set_dry(float value);
void reverb_set_width(float value);
void reverb_process_replace_stereo(float *interleaved, int nframes, int channels);

#endif
