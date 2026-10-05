#ifndef MP3_H 
#define MP3_H 
#include "eng_pipewire.h"
struct voice;
struct sample;

int load_mp3(const char *filename, struct voice *data,int debug);
int load_mp3_sample(const char*sample_path,struct sample *data,int debug);
int play_loaded_mp3_sample(struct sample *s,struct voice *data,int debug);

#endif
