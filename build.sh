#!bin/bash

gcc -fPIC -shared aten.c eng/eng_pipewire.c eng/wav.c eng/mp3.c eng/effects/reverb/reverb.c -o libatenaudio.so -lpthread $(pkg-config --cflags --libs libpipewire-0.3) -lm -Ieng
sudo cp libatenaudio.so /usr/local/lib/
sudo ldconfig
