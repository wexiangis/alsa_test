#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "play.h"

#define TEST 3

#if(TEST == 3)

#define RECORD_WAV "./capture.wav"

void fun(SNDPCMContainer2_t *playback2)
{
    // circle_play_load_wav(playback2, "./music.wav");
    circle_play_load_wav(playback2, "./music2.wav");
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

#elif(TEST == 2)

void fun(void)
{
    // record_wav(TC_WAV, 3);

    // play_wav(TT_WAV);
}

int main()
{
    pthread_t th;

    // sys_volume_set(10);

    pthread_create(&th, NULL, (void*)&fun, NULL);

    play_wav(T1_WAV);
    // play_wav(TP_WAV);
    // play_wav(TC_WAV);
    // play_wav(TT_WAV);

    pthread_join(th, NULL);
    return 0;
}

#elif(TEST == 1)

int main()   
{  
    int val;  
  
    printf("ALSA library version: %s\n",  
                       SND_LIB_VERSION_STR);  
  
    printf("\nPCM stream types:\n");  
    for (val = 0; val <= SND_PCM_STREAM_LAST; val++)  
            printf(" %s\n",  
                snd_pcm_stream_name((snd_pcm_stream_t)val));  
  
    printf("\nPCM access types:\n");  
    for (val = 0; val <= SND_PCM_ACCESS_LAST; val++)  
    {  
            printf(" %s\n",  
                snd_pcm_access_name((snd_pcm_access_t)val));  
    }  
  
    printf("\nPCM formats:\n");  
    for (val = 0; val <= SND_PCM_FORMAT_LAST; val++)  
        {  
        if (snd_pcm_format_name((snd_pcm_format_t)val)!= NULL)  
        {  
                printf(" %s (%s)\n",  
                    snd_pcm_format_name((snd_pcm_format_t)val),  
                    snd_pcm_format_description(  
                            (snd_pcm_format_t)val));  
        }  
    }  
    printf("\nPCM subformats:\n");  
    for (val = 0; val <= SND_PCM_SUBFORMAT_LAST;val++)  
        {  
        printf(" %s (%s)\n",  
                snd_pcm_subformat_name((  
                snd_pcm_subformat_t)val),  
                snd_pcm_subformat_description((  
                snd_pcm_subformat_t)val));  
    }  
    printf("\nPCM states:\n");  
    for (val = 0; val <= SND_PCM_STATE_LAST; val++)  
            printf(" %s\n",  
                snd_pcm_state_name((snd_pcm_state_t)val));  
  
    return 0;  
}

#elif(TEST == 0)

int main()
{
    int val;
    printf("ALSA library version: %s\n",
    SND_LIB_VERSION_STR);
    printf("\nPCM stream types:\n");
    for (val = 0; val <= SND_PCM_STREAM_LAST; val++)
    printf(" %s\n",
    snd_pcm_stream_name((snd_pcm_stream_t)val));
    printf("\nPCM access types:\n");
    for (val = 0; val <= SND_PCM_ACCESS_LAST; val++)
    {
    printf(" %s\n",
    snd_pcm_access_name((snd_pcm_access_t)val));
    }
    printf("\nPCM formats:\n");
    for (val = 0; val <= SND_PCM_FORMAT_LAST; val++)
    {
    if (snd_pcm_format_name((snd_pcm_format_t)val)!= NULL)
    {
    printf(" %s (%s)\n",
    snd_pcm_format_name((snd_pcm_format_t)val),
    snd_pcm_format_description(
    (snd_pcm_format_t)val));
    }
    }
    printf("\nPCM subformats:\n");
    for (val = 0; val <= SND_PCM_SUBFORMAT_LAST;val++)
    {
    printf(" %s (%s)\n",
    snd_pcm_subformat_name((
    snd_pcm_subformat_t)val),
    snd_pcm_subformat_description((
    snd_pcm_subformat_t)val));
    }
    printf("\nPCM states:\n");
    for (val = 0; val <= SND_PCM_STATE_LAST; val++)
    printf(" %s\n",
    snd_pcm_state_name((snd_pcm_state_t)val));
    return 0;
}

#endif

