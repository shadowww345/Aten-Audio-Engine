#ifndef ATEN_H
#define ATEN_H

struct sample;
struct voice;

int set_volume(int volume);
float get_volume();
int set_channels(int channels);
int get_channels();
int set_samplerate(int samplerate);
int get_samplerate();
int reverb(int voice_id, float roomsize,float damp,float wet,float dry,float width);
int set_voice_reverb(int voice_id, int enabled);
int playsound(const char *format,const char *name);
int ateninit();
void set_appname(const char*new);
const char *get_appname();
int set_debug(int debug);
int get_debug(); 
void openreverb();
int stop_sound(int voice_id);
int stop_loop(int voice_id);
int get_loop(int voice_id);
int set_loop(int voice_id, int enabled);
int set_voice_volume(int voice_id, float volume);
int stop_engine(void);
void set_reverb(int rv);
int get_reverb(int voice_id);
int is_ended(int voice_id);
int load_sample(const char*format,const char*path);
void unload_sample(struct sample *sample);
int play_sample(const char*format,int sample_id);
int echo(int voice_id, float delay_ms, float feedback, float damp, float wet, int pingpong);
int set_voice_echo(int voice_id, int enabled);
void set_echo(int on);
int  get_voice_echo(int voice_id);
int get_spectrum(float *bands, float *peaks, int n_bands);
int pause_voice(int voice_id);
int resume_voice(int voice_id);
int is_paused(int voice_id);

#endif
