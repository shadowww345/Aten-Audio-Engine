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