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

## Using:
### LuaJIT implementation:
````lua
local aten= ffi.load("atenaudio")
ffi.cdef[[
   int set_volume(int volume);
   float get_volume();
   int set_channels(int channels);
   int get_channels();
   int set_samplerate(int samplerate);
   int get_samplerate();
   int set_loop(int enabled);
   int get_loop();
   int stop_loop();
   int stop_sound();
   int stop_engine();
   int reverb(float roomsize,float damp,float wet,float dry,float width);
   int playsound(const char *format,const char *name);
   int ateninit();
   void reverb_init(void);
   void set_reverb(int rv);
   int get_reverb();
   void reverb_set_roomsize(float value);
   void reverb_set_damp(float value);
   void reverb_set_wet(float value);
   void reverb_set_dry(float value);
   void reverb_set_width(float value);
]]
aten.set_volume(1)
aten.playsound("mp3","music.mp3")
aten.reverb(0.6,0.3,0.35,0.6,1.0)
aten.ateninit()
````
````C
#include "aten.h"

int main() {
  set_volume(1.0f);
  playsound("wav","music.wav");
  reverb(0.6,0.3,0.35,0.6,1.0);
  ateninit();
}
````
**Run**
````bash
gcc aten.c atenimp.c eng/eng_pipewire.c eng/wav.c eng/mp3.c eng/effects/reverb/reverb.c -o aten -lpthread $(pkg-config --cflags --libs libpipewire-0.3) -lm -Ieng
````
### Using Effects
#### Reverb:
**Argument 0:Roomsize(Float)** <br>
**Argument 1:Damp(Float)** <br>
**Argument 2:Wet(Float)** <br>
**Argument 3:Dry(Float)** <br>
**Argument 4:Width(Float)** <br>
````C
reverb(0.6,0.3,0.35,0.6,1.0);
````
#### Others:
#### Initing Reverb
````C
reverb_init();
````
#### Set/Getting Reverb
**Argument 0:Reverb(Integer(1/0))** <br>
**NOTE:This function is not directly open Reverb. This function is setting engine's data struct int reverb variable** <br>
**NOTE:If you do not call reverb_init() after using this function, the program will crash.** <br>
````C
set_reverb(1);
get_reverb();
````
#### Setting Roomsize
**Argument 0:Roomsize(Float)** <br>
````C
reverb_set_roomsize(0.6f);
````
#### Setting Damp
**Argument 0:Damp(Float)** <br>
````C
reverb_set_damp(0.3f);
````
#### Setting Dry
**Argument 0:Dry(Float)** <br>
````C
reverb_set_dry(0.35f);
````
#### Setting Width
**Argument 0:Width(Float)** <br>
````C
reverb_set_width(0.6f);
````
### Using Functions
#### İniting Engine
````C
ateninit();
````
#### Set/Getting Volume
**Argument 0:Volume(Float)** <br>
````C
set_volume(1.0f);
get_volume();
````
#### Set/Getinng Channels
**Argument 0:Channels(Integer)** <br>
````C
set_channels(1);
get_volume();
````
#### Set/Getinng Sample Rate
**Argument 0:Sample Rate(Integer)** <br>
````C
set_samplerate(44100);
get_samplerate();
````
#### Set/Getinng Loop
**Argument 0:Enabled(Integer(1/0))** <br>
````C
set_loop(44100);
get_loop();
````
#### Stop Loop
````C
stop_loop();
````
#### Stop Sound
````C
stop_sound();
````
#### Stop Engine
```C
stop_engine();
````
