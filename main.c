#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "wmix_user.h"

#define RECORD_WAV "./capture.wav"

void fun(void)
{
    // wmix_play_wav("./music.wav");
}

int main()
{
    char input[16];
    pthread_t th;

    pthread_create(&th, NULL, (void*)&fun, NULL);

    while(1)
    {
        if(scanf("%s", input) > 0)
        {
            if(input[0] == '1')
                wmix_play_wav("./test.wav");
            else if(input[0] == '2')
                wmix_play_wav("./test2.wav");
            else if(input[0] == '3')
                wmix_play_wav("./test3.wav");

            else if(input[0] == 's' && input[1] == '1')
                wmix_play_wav("./sin.wav");
            else if(input[0] == 's' && input[1] == '2')
                wmix_play_wav("./sin2.wav");
            else if(input[0] == 's' && input[1] == '3')
                wmix_play_wav("./sin3.wav");
            else if(input[0] == 's' && input[1] == '4')
                wmix_play_wav("./sin4.wav");

            else if(input[0] == 'm' && input[1] == '1')
                wmix_play_wav("./music.wav");
            else if(input[0] == 'm' && input[1] == '2')
                wmix_play_wav("./music2.wav");
            
            //设置音量
            else if(input[0] == 'v')
                wmix_set_volume(input[1] - '0', 10);

            else if(input[0] == 'q')
                break;
        }
        usleep(10000);
    }
    //
    return 0;
}
