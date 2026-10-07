#ifndef RESAMPLE_H
#define RESAMPLE_H
#include <stdint.h>

struct voice;

int resample_voice(struct voice *v, uint32_t target_rate);

#endif