#include "eng_pipewire.h"
#include "wav.h"
#include <effects/reverb/reverb.h>

float DEFAULT_VOLUME = 0.7f;

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

    int16_t *src = (int16_t *)(data->audio_data + data->data_pos);
    
    int frames_to_copy = n_frames;
    uint32_t remaining_frames = (data->data_size - data->data_pos) / (sizeof(int16_t) * data->channels);
    
    if (frames_to_copy > remaining_frames) {
        frames_to_copy = remaining_frames;
    }
    for (int i = 0; i < frames_to_copy * data->channels; i++) {
        dst[i] = (src[i] / 32768.0f) * DEFAULT_VOLUME;
    }

    if (frames_to_copy < n_frames) {
        for (int i = frames_to_copy * data->channels; i < n_frames * data->channels; i++) {
            dst[i] = 0.0f;
        }
    }

    data->data_pos += frames_to_copy * (sizeof(int16_t) * data->channels);

    if (data->data_pos >= data->data_size) {
        data->data_pos = 0; 
    }
    if (data->channels == 2)
        reverb_process_replace_stereo(dst, n_frames, data->channels);

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