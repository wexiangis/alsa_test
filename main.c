#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "play.h"

#define TEST 2

#if(TEST == 2)

#define T1_WAV "./sin.wav"
#define T2_WAV "./sin2.wav"

#define TP_WAV "./create.wav"

#define TC_WAV "./capture.wav"

void fun(void)
{
    usleep(100000);
    // play_wav(T4_WAV);
    // record_wav(TC_WAV, 1);
}

int main()
{
    pthread_t th;

    // sys_volume_set(10);

    pthread_create(&th, NULL, (void*)&fun, NULL);

    usleep(100000);

    play_wav(T1_WAV);
    // play_wav(TC_WAV);

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

