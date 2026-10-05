#include <stdio.h>
#include <math.h>
#include <eng_pipewire.h>
#include <wav.h>
#include <effects/reverb/reverb.h>
#include <effects/echo/echo.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mp3.h>
#include <resample.h>
#include "aten.h"
#include <effects/spectrum/spectrum.h>

static struct data g_data = { 0, };
static pthread_t g_audio_thread;
int idebug = 0;
const char *app_name="aten-audio";

void set_appname(const char*new) {
    app_name=new;
    return;
} 
//its working :D run pw-top while app running
const char *get_appname() {
    return app_name;
}
//its working in new api but just boosts sound
int set_volume(int volume) {
    DEFAULT_VOLUME=volume;
    return 0;
}

int set_debug(int debug) {
    idebug=debug;
    return idebug;
}

int get_debug() {
    return idebug;
}

float get_volume() {
    return DEFAULT_VOLUME;
}

int pause_voice(int voice_id) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].paused, 1);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].paused, 1);
    return 0;
}

int resume_voice(int voice_id) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].paused, 0);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].paused, 0);
    return 0;
}

int is_paused(int voice_id) {
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return 0;
    return atomic_load(&g_data.voices[voice_id].paused);
}

int set_channels(int channels) {
    DEFAULT_CHANNELS=channels;
    return 0;
}
// new api. master switch disables/enables reverb processing globally overriding per voice reverb_send
void set_reverb(int rv) {
    g_data.reverb=rv;
}
int get_reverb(int voice_id) {
    return atomic_load(&g_data.voices[voice_id].reverb_send);
}
int get_channels() {
    return DEFAULT_CHANNELS;
}

int set_samplerate(int samplerate) {
    DEFAULT_RATE=samplerate;
    return 0;
}

int get_samplerate() {
    return DEFAULT_RATE;
}

int is_ended(int voice_id) {
    return atomic_load(&g_data.voices[voice_id].finished);
}

static int g_reverb_initialized = 0;

int reverb(int voice_id, float roomsize,float damp,float wet,float dry,float width) {
    if (!g_reverb_initialized) {
        reverb_init();
        g_reverb_initialized = 1;
    }
    reverb_set_roomsize(roomsize);
    reverb_set_damp(damp);
    reverb_set_wet(wet);
    reverb_set_dry(dry);
    reverb_set_width(width);

    g_data.reverb = 1;
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].reverb_send, 1);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].reverb_send, 1);
    return 0;
}
int set_voice_reverb(int voice_id, int enabled) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].reverb_send, enabled);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].reverb_send, enabled);
    return 0;
}
//master reverb open switch in new api see set_reverb for info
void openreverb() {
    g_data.reverb=1;
}

int get_spectrum(float *bands, float *peaks, int n_bands) {
    return spectrum_get_bands(bands, peaks, n_bands);
}

static int g_echo_initialized = 0;

int echo(int voice_id, float delay_ms, float feedback, float damp, float wet, int pingpong) {
    if (!g_echo_initialized) {
        if (echo_init(DEFAULT_RATE) != 0) return -1;
        g_echo_initialized = 1;
    }
    echo_set_delay(delay_ms);
    echo_set_feedback(feedback);
    echo_set_damp(damp);
    echo_set_wet(wet);
    echo_set_pingpong(pingpong);

    g_data.echo = 1;
    return set_voice_echo(voice_id, 1);
}

int set_voice_echo(int voice_id, int enabled) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].echo_send, enabled);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES) return -1;
    atomic_store(&g_data.voices[voice_id].echo_send, enabled);
    return 0;
}

void set_echo(int on)  { g_data.echo = on; }   // master switch
int  get_voice_echo(int voice_id) { return atomic_load(&g_data.voices[voice_id].echo_send); }

static int match_channels(struct voice *v, int target_channels) {
    if (v->channels == (uint16_t)target_channels)
        return 0;

    uint32_t src_frames = v->data_size / (sizeof(int16_t) * v->channels);
    int16_t *src = (int16_t *)v->audio_data;
    int16_t *out = malloc((size_t)src_frames * (size_t)target_channels * sizeof(int16_t));
    if (out == NULL) {
        printf("\e[1;31m[ERROR]\e[0mMixer: out of memory converting channels\n");
        return -1;
    }

    if (v->channels == 1 && target_channels == 2) {
        for (uint32_t i = 0; i < src_frames; i++) {
            out[i*2 + 0] = src[i];
            out[i*2 + 1] = src[i];
        }
    } else if (v->channels == 2 && target_channels == 1) {
        for (uint32_t i = 0; i < src_frames; i++) {
            out[i] = (int16_t)(((int32_t)src[i*2] + (int32_t)src[i*2 + 1]) / 2);
        }
    } else {
        printf("\e[1;31m[ERROR]\e[0mMixer: cannot convert %d channel(s) to %d, skipping sound\n",
               v->channels, target_channels);
        free(out);
        return -1;
    }

    free(v->audio_data);
    v->audio_data = (uint8_t *)out;
    v->data_size  = src_frames * (uint32_t)target_channels * (uint32_t)sizeof(int16_t);
    v->channels   = (uint16_t)target_channels;
    return 0;
}

static int alloc_voice(void) {
    int found = -1;
    pthread_mutex_lock(&g_data.alloc_lock);
    for (int i = 0; i < MAX_VOICES; i++) {
        if (atomic_load(&g_data.voices[i].state) == VOICE_FREE) {
            if (g_data.voices[i].audio_data) {
                free(g_data.voices[i].audio_data);
                g_data.voices[i].audio_data = NULL;
                g_data.voices[i].data_size = 0;
            }
            atomic_store(&g_data.voices[i].state, VOICE_LOADING);
            found = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_data.alloc_lock);
    return found;
}

static int alloc_sample() {
    int found = -1;
    pthread_mutex_lock(&g_data.alloc_lock);
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (atomic_load(&g_data.samples[i].loaded) == 0) {
            if (g_data.samples[i].audio_data) {
                free(g_data.samples[i].audio_data);
                g_data.samples[i].audio_data = NULL;
                g_data.samples[i].data_size = 0;
            }
            atomic_store(&g_data.samples[i].playing, 1);
            found = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_data.alloc_lock);
    return found;
}

static void release_voice(struct voice *v) {
    if (v->audio_data) {
        free(v->audio_data);
        v->audio_data = NULL;
    }
    v->data_size = 0;
    atomic_store(&v->state, VOICE_FREE);
}

void unload_sample(struct sample *sample) {
    if (sample->audio_data)
    {
        free(sample->audio_data);
        sample->audio_data=NULL;
    }
    atomic_store(&sample->loaded,0);
    sample->data_size = 0;
}
int load_sample(const char*format,const char*path) {
    int idx = alloc_sample();
    if(idx<0) {
        printf("\e[1;31m[ERROR]\e[0mMixer: no free sample slots (MAX_SAMPLES=%d)\n", MAX_SAMPLES);
    }
    struct sample *s = &g_data.samples[idx];
    int rc;
    if(strcmp(format,"wav")==0) {
        rc = load_wav_sample(path,s,idebug);
    }
    else if(strcmp(format,"mp3")==0) {
        load_mp3_sample(path,s,idebug);
        return 0;
    }
    else {
        printf("\e[1;31m[ERROR]\e[0mFormat:Unsupported audio format. supported formats:wav('wav') mp3('mp3')\n");
        return -1;
    }
    return idx;
}
int play_sample(const char*format,int sample_id) {
    struct sample *s = &g_data.samples[sample_id];
    int idx = alloc_voice();
    struct voice *v = &g_data.voices[idx];
    if (idx < 0) {
        printf("\e[1;31m[ERROR]\e[0mMixer: no free voice slots (MAX_VOICES=%d)\n", MAX_VOICES);
        return -1;
    }
    if(strcmp(format,"wav")==0) {
        play_loaded_wav_sample(s,v,idebug);
    }
    else if(strcmp(format,"mp3")==0) {
        play_loaded_mp3_sample(s,v,idebug);
    }
    else {
        printf("\e[1;31m[ERROR]\e[0mFormat:Unsupported audio format. supported formats:wav('wav') mp3('mp3'))\n");
        return -1;
    }
    if (resample_voice(v, (uint32_t)DEFAULT_RATE) != 0) {
        printf("\e[1;31m[ERROR]\e[0mMixer: resample failed\n");
        release_voice(v);
        return -1;
    }
    if (match_channels(v, DEFAULT_CHANNELS) != 0) {
        release_voice(v);
        return -1;
    }

    v->volume = 1.0f;
    v->data_pos = 0;
    atomic_store(&v->finished, 0);
    atomic_store(&v->loop_enabled, 0);
    atomic_store(&v->reverb_send, 0);
    atomic_store(&v->echo_send, 0);
    atomic_store(&v->paused, 0);
    v->fade = 1.0f;
    atomic_store(&v->state, VOICE_PLAYING);
    return idx;
}

int playsound(const char *format,const char *name) {
    int idx = alloc_voice();
    if (idx < 0) {
        printf("\e[1;31m[ERROR]\e[0mMixer: no free voice slots (MAX_VOICES=%d)\n", MAX_VOICES);
        return -1;
    }
    struct voice *v = &g_data.voices[idx];

    int rc;
    if(strcmp(format,"wav")==0) {
        rc = load_wav(name, v,idebug);
    }
    else if(strcmp(format,"mp3")==0) {
        rc = load_mp3(name, v,idebug);
    }
    else {
        printf("\e[1;31m[ERROR]\e[0mFormat:Invalid format or no format entered\n");
        rc = -1;
    }

    if (rc != 0) {
        release_voice(v);
        return -1;
    }

    if (resample_voice(v, (uint32_t)DEFAULT_RATE) != 0) {
        printf("\e[1;31m[ERROR]\e[0mMixer: resample failed\n");
        release_voice(v);
        return -1;
    }
    if (match_channels(v, DEFAULT_CHANNELS) != 0) {
        release_voice(v);
        return -1;
    }

    v->volume = 1.0f;
    v->data_pos = 0;
    atomic_store(&v->finished, 0);
    atomic_store(&v->loop_enabled, 0);
    atomic_store(&v->reverb_send, 0);
    atomic_store(&v->echo_send, 0);
    atomic_store(&v->paused, 0);
    v->fade = 1.0f;
    atomic_store(&v->state, VOICE_PLAYING);
    return idx;
}
int stop_sound(int voice_id) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].finished, 1);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].finished, 1);
    return 0;
}

int stop_loop(int voice_id) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].loop_enabled, 0);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].loop_enabled, 0);
    return 0;
}

int get_loop(int voice_id) {
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    return atomic_load(&g_data.voices[voice_id].loop_enabled);
}

int set_loop(int voice_id, int enabled) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            atomic_store(&g_data.voices[i].loop_enabled, enabled);
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    atomic_store(&g_data.voices[voice_id].loop_enabled, enabled);
    return 0;
}

int set_voice_volume(int voice_id, float volume) {
    if (voice_id == -1) {
        for (int i = 0; i < MAX_VOICES; i++)
            g_data.voices[i].volume = volume;
        return 0;
    }
    if (voice_id < 0 || voice_id >= MAX_VOICES)
        return -1;
    g_data.voices[voice_id].volume = volume;
    return 0;
}

static void *audio_thread_fn(void *arg) {
    (void)arg;
    const struct spa_pod *params[1];
    uint32_t n_params = 0;
    uint8_t buffer[1024];
    struct pw_properties *props;
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    g_data.channels=DEFAULT_CHANNELS;
    pw_init(NULL, NULL);
    g_data.loop = pw_main_loop_new(NULL);

    pw_loop_add_signal(pw_main_loop_get_loop(g_data.loop), SIGINT, do_quit, &g_data);
    pw_loop_add_signal(pw_main_loop_get_loop(g_data.loop), SIGTERM, do_quit, &g_data);

    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                    PW_KEY_MEDIA_CATEGORY, "Playback",
                    PW_KEY_MEDIA_ROLE, "Music",
                    NULL);
    if (1)
        pw_properties_set(props, PW_KEY_TARGET_OBJECT, NULL);
    g_data.stream = pw_stream_new_simple(
                    pw_main_loop_get_loop(g_data.loop),
                        app_name,
                        props,
                        &stream_events,
                        &g_data);
    params[n_params++] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                    &SPA_AUDIO_INFO_RAW_INIT(
                            .format = SPA_AUDIO_FORMAT_F32,
                            .channels = DEFAULT_CHANNELS,
                            .rate = DEFAULT_RATE ));
    pw_stream_connect(g_data.stream,
                        PW_DIRECTION_OUTPUT,
                        PW_ID_ANY,
                        PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS |
                        PW_STREAM_FLAG_RT_PROCESS,
                        params, n_params);
    pw_main_loop_run(g_data.loop);

    pw_stream_destroy(g_data.stream);
    pw_main_loop_destroy(g_data.loop);
    pw_deinit();
    return NULL;
}

int ateninit() {
    pthread_mutex_init(&g_data.alloc_lock, NULL);
    for (int i = 0; i < MAX_VOICES; i++) {
        g_data.voices[i].volume = 1.0f;
        atomic_store(&g_data.voices[i].state, VOICE_FREE);
    }
    spectrum_init(DEFAULT_RATE);
    return pthread_create(&g_audio_thread, NULL, audio_thread_fn, NULL);
}

int stop_engine(void) {
    if (g_data.loop) {
        pw_main_loop_quit(g_data.loop);
        pthread_join(g_audio_thread, NULL);
    }
    for (int i = 0; i < MAX_VOICES; i++) {
        if (g_data.voices[i].audio_data) {
            free(g_data.voices[i].audio_data);
            g_data.voices[i].audio_data = NULL;
        }
    }
    pthread_mutex_destroy(&g_data.alloc_lock);
    return 0;
}
