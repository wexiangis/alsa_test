#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "play.h"

#define RECORD_WAV "./capture.wav"

void fun(SNDPCMContainer2_t *playback2)
{
    // circle_play_load_wav(playback2, "./music.wav");
    // circle_play_load_wav(playback2, "./music2.wav");
    // usleep(1500);
    // circle_play_load_wav(playback2, "./music2.wav");
    // circle_play_load_wav(playback2, RECORD_WAV);
}

int main()
{
    char input[16];
    pthread_t th;

    // sys_volume_set(10);

    // record_wav(RECORD_WAV, 5);

    SNDPCMContainer2_t *playback2 = circle_play_init();

    if(!playback2)
    {
        fprintf(stderr, "playback2 init err\n");
        return -1;
    }
    
    /////

    pthread_create(&th, NULL, (void*)&fun, (void*)playback2);

    while(1)
    {
        if(scanf("%s", input) > 0)
        {
            if(input[0] == '1')
                circle_play_load_wav(playback2, "./test.wav");
            else if(input[0] == '2')
                circle_play_load_wav(playback2, "./test2.wav");
            else if(input[0] == '3')
                circle_play_load_wav(playback2, "./test3.wav");

            else if(input[0] == 's' && input[1] == '1')
                circle_play_load_wav(playback2, "./sin.wav");
            else if(input[0] == 's' && input[1] == '2')
                circle_play_load_wav(playback2, "./sin2.wav");
            else if(input[0] == 's' && input[1] == '3')
                circle_play_load_wav(playback2, "./sin3.wav");
            else if(input[0] == 's' && input[1] == '4')
                circle_play_load_wav(playback2, "./sin4.wav");

            else if(input[0] == 'm' && input[1] == '1')
                circle_play_load_wav(playback2, "./music.wav");
            else if(input[0] == 'm' && input[1] == '2')
                circle_play_load_wav(playback2, "./music2.wav");

            else if(input[0] == 'q')
                break;
        }
        usleep(10000);
    }

    /////

    circle_play_exit(playback2);

    return 0;
}
