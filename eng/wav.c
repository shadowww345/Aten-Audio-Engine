#include "wav.h"

int load_wav(const char *path,struct data *app) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[ERROR]File: File not opened\n");
        return -1;
    }

    WAV_H hdr;

    if (fread(&hdr.riff,   1, 4,  f) != 4  ||
        fread(&hdr.file_size, 1, 4,  f) != 4  ||
        fread(&hdr.wave,   1, 4,  f) != 4) {
        fprintf(stderr, "[ERROR]WAV:RIFF header read error\n");
        fclose(f); return -1;
    }
    if (memcmp(hdr.riff, "RIFF", 4) || memcmp(hdr.wave, "WAVE", 4)) {
        fprintf(stderr, "[ERROR]File: is not valid wav file\n");
        fclose(f); return -1;
    }

    int got_fmt = 0, got_data = 0;
    while (!got_fmt || !got_data) {
        char     chunk_id[4];
        uint32_t chunk_size;
        if (fread(chunk_id,    1, 4, f) != 4) break;
        if (fread(&chunk_size, 1, 4, f) != 4) break;

        if (!memcmp(chunk_id, "fmt ", 4)) {
            uint8_t buf[40];
            uint32_t read_size = chunk_size < sizeof(buf) ? chunk_size : sizeof(buf);
            if (fread(buf, 1, read_size, f) != read_size) break;
            if (chunk_size > read_size) fseek(f, chunk_size - read_size, SEEK_CUR);

            hdr.audio_format    = *(uint16_t *)(buf + 0);
            hdr.channels        = *(uint16_t *)(buf + 2);
            hdr.sample_rate     = *(uint32_t *)(buf + 4);
            hdr.byte_rate       = *(uint32_t *)(buf + 8);
            hdr.block_align     = *(uint16_t *)(buf + 12);
            hdr.bits_per_sample = *(uint16_t *)(buf + 14);
            got_fmt = 1;

        } else if (!memcmp(chunk_id, "data", 4)) {
            app->data_size = chunk_size;
            app->audio_data = malloc(chunk_size);
            if (!app->audio_data) {
                fprintf(stderr, "[ERROR]Memory:Out of memory\n");
                fclose(f); return -1;
            }
            if (fread(app->audio_data, 1, chunk_size, f) != chunk_size) {
                fprintf(stderr, "[ERROR]File: Sound data read error\n");
                free(app->audio_data); fclose(f); return -1;
            }
            got_data = 1;

        } else {
            fseek(f, chunk_size, SEEK_CUR);
        }
    }
    fclose(f);

    if (!got_fmt || !got_data) {
        fprintf(stderr, "[Error]WAV:fmt or data chunk not found\n");
        return -1;
    }
    if (hdr.audio_format != 1) {
        fprintf(stderr, "[ERROR]File: Unsupported sound format\n");
        return -1;
    }

    app->channels        = hdr.channels;
    app->sample_rate     = hdr.sample_rate;
    app->bits_per_sample = hdr.bits_per_sample;
    app->data_pos        = 0;

    printf("[OK]File loaded   :\n");
    printf("  Type            : Wav\n");
    printf("  Channels        : %u\n", app->channels);
    printf("  Sample Rate     : %u Hz\n", app->sample_rate);
    printf("  Bits per sample : %u-bit\n", app->bits_per_sample);
    printf("  File Size       : %u byte\n", app->data_size);
    printf("\n");
    double duration = (double)app->data_size /
                      (app->sample_rate * app->channels * (app->bits_per_sample / 8));
    printf("  Time            : %.2f second\n\n", duration);

    return 0;
}
