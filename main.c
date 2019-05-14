#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wmix_user.h"

#define RECORD_WAV "./capture.wav"

void fun(void)
{
    // wmix_play_wav("./music.wav");

    int fd;
    int ret;
    uint8_t buff[2048];
    WMix_Stream *stream = wmix_stream_init(2, 16, 44100);
    if(stream)
    {
        fd = open("./music.wav", O_RDONLY);
        if(fd > 0)
        {
            while(1)
            {
                ret = read(fd, buff, 2048);
                if(ret > 0)
                    wmix_stream_transfer(stream, buff, ret);
                else
                    break;
                usleep(1000);
            }
            //
            close(fd);
        }
        //
        sleep(3);
        //
        wmix_stream_release(stream);
    }
}

int main()
{
    WMix_Stream *stream = NULL;
    char input[16];
    pthread_t th;

    // pthread_create(&th, NULL, (void*)&fun, NULL);

    while(1)
    {
        memset(input, 0, sizeof(input));
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
            
            else if(input[0] == 't' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun, NULL);
            
            else if(input[0] == 't' && input[1] == '1')
                stream = wmix_stream_init(2, 16, 44100);
            else if(input[0] == 't' && input[1] == '2')
                wmix_stream_release(stream);

            //设置音量
            else if(input[0] == 'v' && input[1] == '1' && input[2] == '0')
                wmix_set_volume(10, 10);
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
