#include <stdio.h>
#include <math.h>
#include <eng_pipewire.h>
#include <wav.h>
#include <effects/reverb/reverb.h>

int main(int argc, char *argv[]) {
    struct data data = { 0, };
    const struct spa_pod *params[1];
    uint32_t n_params = 0;
    uint8_t buffer[1024];
    struct pw_properties *props;
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    
    pw_init(&argc, &argv);
    data.loop = pw_main_loop_new(NULL);
    if(load_wav(argv[1],&data)< 0) return 1;
    //setting volume
    if (argc > 2) {
        DEFAULT_VOLUME = atof(argv[2]);
    }
    else {
        DEFAULT_VOLUME=0.7;
    }

    reverb_init();
    reverb_set_roomsize(0.6f);
    reverb_set_damp(0.3f);
    reverb_set_wet(0.35f);
    reverb_set_dry(0.6f);
    reverb_set_width(1.0f);

    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
                    PW_KEY_MEDIA_CATEGORY, "Playback",
                    PW_KEY_MEDIA_ROLE, "Music",
                    NULL);
    if (argc > 1)
        pw_properties_set(props, PW_KEY_TARGET_OBJECT, argv[1]);
    data.stream = pw_stream_new_simple(
                    pw_main_loop_get_loop(data.loop),
                        "audio-src",
                        props,
                        &stream_events,
                        &data);
    params[n_params++] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
                    &SPA_AUDIO_INFO_RAW_INIT(
                            .format = SPA_AUDIO_FORMAT_F32,
                            .channels = DEFAULT_CHANNELS,
                            .rate = DEFAULT_RATE ));
    pw_stream_connect(data.stream,
                        PW_DIRECTION_OUTPUT,
                        PW_ID_ANY,
                        PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS |
                        PW_STREAM_FLAG_RT_PROCESS,
                        params, n_params);
    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();
    return 0;
}