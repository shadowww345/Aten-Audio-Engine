#!bin/bash

gcc main.c eng/eng_pipewire.c eng/wav.c eng/effects/reverb/reverb.c -o aten $(pkg-config --cflags --libs libpipewire-0.3) -lm -Ieng

#-I./player