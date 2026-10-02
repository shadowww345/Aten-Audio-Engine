#include "eng_pipewire.h"
#include "wav.h"
#include <effects/reverb/reverb.h>
#include <string.h>

float DEFAULT_VOLUME = 0.7f;
int DEFAULT_RATE = 44100;
int DEFAULT_CHANNELS = 2; 

static inline float clamp_sample(float x)
{
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return x;
}

static inline void release_voice_slot(struct data *data, struct voice *v)
{
    if (!v->owns_audio_data && v->sample_ref >= 0 && v->sample_ref < MAX_SAMPLES) {
        atomic_fetch_sub(&data->samples[v->sample_ref].refcount, 1);
    }
    v->sample_ref = -1;
    atomic_store(&v->state, VOICE_FREE);
}

void on_process(void *userdata)
{
    struct data *data = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    int n_frames, stride;
    float *dst;
    
    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("[WARN]PipeWire: Out of buffers");
        return;
    }
    
    buf = b->buffer;
    if ((dst = buf->datas[0].data) == NULL)
        return;

    stride = sizeof(float) * data->channels;
    n_frames = buf->datas[0].maxsize / stride;

    if (b->requested)
        n_frames = SPA_MIN((int)b->requested, n_frames);
    if (n_frames > MAX_BLOCK_FRAMES)
        n_frames = MAX_BLOCK_FRAMES;

    memset(dst, 0, (size_t)n_frames * stride);
    float wet_send[MAX_BLOCK_FRAMES * 2];
    int any_reverb_send = 0;
    if (data->reverb)
        memset(wet_send, 0, (size_t)n_frames * stride);

    int any_active = 0;

    for (int vi = 0; vi < MAX_VOICES; vi++) {
        struct voice *v = &data->voices[vi];

        if (atomic_load(&v->state) != VOICE_PLAYING)
            continue;

        if (v->audio_data == NULL || v->data_size == 0 ||
            atomic_load(&v->finished) || v->channels != data->channels) {
            release_voice_slot(data, v);
            continue;
        }

        any_active = 1;

        int16_t *src = (int16_t *)(v->audio_data + v->data_pos);
        uint32_t remaining_frames =
            (v->data_size - v->data_pos) / (sizeof(int16_t) * v->channels);

        int frames_to_copy = n_frames;
        if (frames_to_copy > (int)remaining_frames)
            frames_to_copy = (int)remaining_frames;

        float vol = v->volume * DEFAULT_VOLUME;
        int send_this_voice = data->reverb && atomic_load(&v->reverb_send);
        if (send_this_voice)
            any_reverb_send = 1;

        for (int i = 0; i < frames_to_copy * data->channels; i++) {
            float s = (src[i] / 32768.0f) * vol;
            dst[i] += s;
            if (send_this_voice)
                wet_send[i] += s;
        }

        v->data_pos += frames_to_copy * (sizeof(int16_t) * v->channels);

        if (v->data_pos >= v->data_size) {
            if (atomic_load(&v->loop_enabled)) {
                v->data_pos = 0;
            } else {
                atomic_store(&v->finished, 1);
                release_voice_slot(data, v);
            }
        }
        
    }

    if (data->reverb && any_reverb_send && data->channels == 2) {
        reverb_process_replace_stereo(wet_send, n_frames, data->channels);
        for (int i = 0; i < n_frames * data->channels; i++) {
            dst[i] += wet_send[i];
        }
    }

    if (any_active || any_reverb_send) {
        for (int i = 0; i < n_frames * data->channels; i++) {
            dst[i] = clamp_sample(dst[i]);
        }
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}

const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};
 
void do_quit(void *userdata, int signal_number)
{
        struct data *data = userdata;
        pw_main_loop_quit(data->loop);
}
