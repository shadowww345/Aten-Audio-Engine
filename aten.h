#ifndef ATEN_H
#define ATEN_H

int set_volume(int volume);
float get_volume();
int set_channels(int channels);
int get_channels();
int set_samplerate(int samplerate);
int get_samplerate();
int reverb(float roomsize,float damp,float wet,float dry,float width);
int playsound(const char *format,const char *name);
int ateninit();
void openreverb();
int stop_sound(void);
int stop_loop(void);
int get_loop(void);
int set_loop(int enabled);

#endif