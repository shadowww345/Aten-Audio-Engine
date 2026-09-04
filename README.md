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
````
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

   void reverb_set_roomsize(float value);
   void reverb_set_damp(float value);
   void reverb_set_wet(float value);
   void reverb_set_dry(float value);
   void reverb_set_width(float value);
   void reverb_process_replace_stereo(float *interleaved, int nframes, int channels);
]]
aten.set_volume(1)
aten.playsound("mp3","kv2005.mp3")
aten.reverb(0.6,0.3,0.35,0.6,1.0)
aten.ateninit()
````
