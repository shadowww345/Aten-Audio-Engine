#ifndef ENG_PIPEWIRE_h
#define ENG_PIPEWIRE_h
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include "wav.h"
#include <spa/param/audio/format-utils.h> 
#include <pipewire/pipewire.h>

#define M_PI_M2f (float)(M_PI+M_PI)

extern int DEFAULT_RATE;
extern int DEFAULT_CHANNELS;
extern float DEFAULT_VOLUME;

#define BUFFER_SIZE             (16*1024)

#define MAX_VOICES 16

#define MAX_BLOCK_FRAMES 4096

enum voice_state {
    VOICE_FREE = 0,
    VOICE_LOADING,
    VOICE_PLAYING,
    VOİCE_PAUSED
};


struct voice {
    uint8_t  *audio_data;
    uint32_t  data_size;
    uint32_t  data_pos;
    uint16_t  channels;
    uint32_t  sample_rate;
    uint16_t  bits_per_sample;
    float     volume;
    int       owns_audio_data;
    int       sample_ref;
    atomic_int paused;
    float      fade;
    atomic_int reverb_send;
    atomic_int echo_send;
    atomic_int loop_enabled;
    atomic_int finished;
    atomic_int state;
};

#define MAX_SAMPLES 128

struct sample {
    uint8_t   *audio_data;
    uint32_t   data_size;
    uint16_t   channels;
    uint32_t   sample_rate;
    uint16_t  bits_per_sample;
    atomic_int refcount;
    atomic_int loaded;
    atomic_int playing;
};

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    struct voice voices[MAX_VOICES];
    struct sample samples[MAX_SAMPLES];
    pthread_mutex_t alloc_lock;
    int reverb;
    int debug;
    int echo;
    uint16_t channels;
    uint32_t sample_rate;
};

void on_process(void *userdata);

extern const struct pw_stream_events stream_events;

void do_quit(void *userdata, int signal_number);

#endif
