# Aten-Audio-Engine
## Aten is a Linux Pipewire Audio Engine

### For build
``
sudo apt install libpipewire-0.3-dev
``
### For run
``
sudo apt install libpipewire-0.3-common
``

### Effects
**Reverb:Freeverb Algorithm**
**Echo:Generic Echo Algorithm**

## Using:
### LuaJIT implementation:
````lua
local ffi = require("ffi")
local aten= ffi.load("atenaudio")
ffi.cdef[[
int set_volume(int volume);
float get_volume();
int set_channels(int channels);
int get_channels();
int set_samplerate(int samplerate);
int get_samplerate();
int reverb(int voice_id, float roomsize,float damp,float wet,float dry,float width);
int set_voice_reverb(int voice_id, int enabled);
int playsound(const char *format,const char *name);
int ateninit();
void set_appname(const char*new);
const char *get_appname();
int set_debug(int debug);
int get_debug(); 
void openreverb();
int stop_sound(int voice_id);
int stop_loop(int voice_id);
int get_loop(int voice_id);
int set_loop(int voice_id, int enabled);
int set_voice_volume(int voice_id, float volume);
int stop_engine(void);
void set_reverb(int rv);
int get_reverb(int voice_id);
int is_ended(int voice_id);
int load_sample(const char*format,const char*path);
int play_sample(const char*format,int sample_id);
int echo(int voice_id, float delay_ms, float feedback, float damp, float wet, int pingpong);
int set_voice_echo(int voice_id, int enabled);
void set_echo(int on);
int  get_voice_echo(int voice_id);
int get_spectrum(float *bands, float *peaks, int n_bands);
int pause_voice(int voice_id);
int resume_voice(int voice_id);
int is_paused(int voice_id);
]]
aten.ateninit()
music=aten.playsound("mp3","music.mp3")
aten.reverb(music,0.6,0.3,0.35,0.6,1.0),
while 1 do end
aten.stop_engine()
````
````C
#include <stdio.h>
#include <unistd.h>
#include "aten.h"

int main(void) {
    set_samplerate(44100);
    set_channels(2);
    set_debug(1);
    set_appname("Aten Audio");
    printf("Welcome To ");
    printf("%s",get_appname());
    printf("\n");
    ateninit();
    int music = playsound("mp3","sound/oglumun_tabancasi.mp3");
    set_voice_volume(music,0.7f);
    reverb(music,0.6,0.3,0.35,0.6,1.0);
    echo(music, 350.0f, 0.4f, 0.3f, 0.5f, 1);
    while(1) {
    sleep(1);
    if(is_ended(music)) {
        stop_engine();
        return 0;
    }
    }
    return 0;
}
````
**Run**
````bash
gcc -O3 aten.c test.c eng/eng_pipewire.c eng/wav.c eng/mp3.c eng/effects/reverb/reverb.c eng/resample.c eng/effects/echo/echo.c eng/effects/spectrum/spectrum.c -o player -lpthread $(pkg-config --cflags --libs libpipewire-0.3) -lm -Ieng
````

