#include <stdio.h>
#include <math.h>
#include <eng_pipewire.h>
#include <wav.h>
#include <effects/reverb/reverb.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>

static struct data g_data = { 0, };
static pthread_t g_audio_thread;

int set_volume(int volume) {
    DEFAULT_VOLUME=volume;
    return 0;
}

float get_volume() {
    return DEFAULT_VOLUME;
}

int set_channels(int channels) {
    DEFAULT_CHANNELS=channels;
    return 0;
}

void set_reverb(int rv) {
    g_data.reverb=rv;
}
int get_reverb() {
    return g_data.reverb;
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

int set_loop(int enabled) {
    g_data.loop_enabled = enabled;
    return 0;
}

int get_loop(void) {
    return g_data.loop_enabled;
}

int stop_loop(void) {
    g_data.loop_enabled = 0;
    return 0;
}

int stop_sound(void) {
    g_data.finished = 1;
    return 0;
}
void openreverb() {
    g_data.reverb=1;
}
int reverb(float roomsize,float damp,float wet,float dry,float width) {
    g_data.reverb=1;
    reverb_init();
    reverb_set_roomsize(roomsize);
    reverb_set_damp(damp);
    reverb_set_wet(wet);
    reverb_set_dry(dry);
    reverb_set_width(width);
    return 0;
}

int playsound(const char *format,const char *name) {
    if(strcmp(format,"wav")==0) {
        load_wav(name,&g_data);
        g_data.data_pos = 0;
        g_data.finished = 0;
    }
    else if(strcmp(format,"mp3")==0) {
        load_mp3(name,&g_data);
        g_data.data_pos = 0;
        g_data.finished = 0;
    }
    else {
        printf("Invalid format or no format entered\n");
    }
    return 0;
}
static void *audio_thread_fn(void *arg) {
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
                        "audio-src",
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
    return pthread_create(&g_audio_thread, NULL, audio_thread_fn, NULL);
}

int stop_engine(void) {
    if (g_data.loop) {
        pw_main_loop_quit(g_data.loop);
        pthread_join(g_audio_thread, NULL);
    }
    return 0;
}
