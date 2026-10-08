#!bin/bash

gcc -O3 aten.c test.c eng/eng_pipewire.c eng/wav.c eng/mp3.c eng/effects/reverb/reverb.c eng/resample.c eng/effects/echo/echo.c eng/effects/spectrum/spectrum.c -o player -lpthread $(pkg-config --cflags --libs libpipewire-0.3) -lm -Ieng

#-I./player