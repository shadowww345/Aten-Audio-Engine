#define DR_MP3_IMPLEMENTATION
#include "libs/dr_mp3.h"
#include "mp3.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mp3.h"

int load_mp3(const char *filename, struct data *data) {
    drmp3 mp3;

    if (!drmp3_init_file(&mp3, filename, NULL)) {
        printf("[ERROR]MP3: MP3 open failure: %s\n", filename);
        return -1;
    }

    drmp3_uint64 frame_count = drmp3_get_pcm_frame_count(&mp3);
    if (frame_count == 0) {
        printf("[ERROR]MP3: MP3 Header Error: %s\n", filename);
        drmp3_uninit(&mp3);
        return -1;
    }

    int channels   = mp3.channels;
    int samplerate = mp3.sampleRate;

    size_t total_samples = (size_t)frame_count * (size_t)channels;
    int16_t *pcm = (int16_t *)malloc(total_samples * sizeof(int16_t));
    if (pcm == NULL) {
        printf("[ERROR]MP3: MP3 Malloc Error\n");
        drmp3_uninit(&mp3);
        return -1;
    }

    drmp3_uint64 frames_read = drmp3_read_pcm_frames_s16(&mp3, frame_count, pcm);
    if (data->audio_data != NULL) {
        free(data->audio_data);
        data->audio_data = NULL;
    }

    data->audio_data = (uint8_t *)pcm;
    data->data_size   = (uint32_t)(frames_read * channels * sizeof(int16_t));
    data->data_pos    = 0;
    data->channels    = channels;

    printf("[OK]File loaded   :\n");
    printf("  Type            : MP3\n");
    printf("  Channels        : %d\n", channels);
    printf("  Sample Rate     : %d Hz\n", samplerate);
    printf("  Frames          : %llu\n", (unsigned long long)frames_read);
    printf("  Time            : %.2f second\n",
           (double)frames_read / (double)samplerate);

    drmp3_uninit(&mp3);
    return 0;
}
