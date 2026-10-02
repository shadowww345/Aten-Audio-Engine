#ifndef WAV_HEADER
#define WAV_HEADER 

#define WAV_HEADER_SIZE 44
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include "eng_pipewire.h"
struct voice;
struct sample;
typedef struct {
    char     riff[4];
    uint32_t file_size;
    char     wave[4];
    char     fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char     data[4];
    uint32_t data_size;
} WAV_H;
// loading playing sample coming soon for mp3
int load_wav(const char *path,struct voice *app,int debug);
int play_loaded_wav_sample(struct sample *s,struct voice *vc);
int load_wav_sample(const char*sample_path,struct sample *app);

#endif
