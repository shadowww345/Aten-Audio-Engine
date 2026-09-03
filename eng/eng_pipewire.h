#ifndef ENG_PIPEWIRE_h
#define ENG_PIPEWIRE_h
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include "wav.h"
#include <spa/param/audio/format-utils.h> 
#include <pipewire/pipewire.h>

#define M_PI_M2f (float)(M_PI+M_PI)
 
#define DEFAULT_RATE            44100
#define DEFAULT_CHANNELS        2
extern float DEFAULT_VOLUME;

#define BUFFER_SIZE             (16*1024)

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    uint8_t  *audio_data;
    uint32_t  data_size;
    uint32_t  data_pos;

    uint16_t  channels;
    uint32_t  sample_rate;
    uint16_t  bits_per_sample;
    float accumulator;
};

void fill_f32(struct data *d, void *dest, int n_frames);
void on_process(void *userdata);

extern const struct pw_stream_events stream_events;

void on_process(void *userdata);
void do_quit(void *userdata, int signal_number);

#endif