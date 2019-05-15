#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wmix_user.h"

void fun(void)
{
    int fd;
    ssize_t ret, total = 0;
    uint8_t buff[4096];

    // int stream = wmix_stream_open(2, 16, 44100);
    int stream = wmix_stream_open(1, 16, 22050);
    if(stream > 0)
    {
        // fd = open("./music.wav", O_RDONLY);
        fd = open("./music2.wav", O_RDONLY);
        if(fd > 0)
        {
            //跳过文件头
            read(fd, buff, 44);
            //
            while(1)
            {
                ret = read(fd, buff, 4096);
                if(ret > 0)
                    write(stream, buff, ret);
                else
                    break;
                total += ret;
            }
            //
            close(fd);
        }
        //
        close(stream);
        //
        printf("wav write end: %ld\n", (long)total);
    }
}

int main()
{
    int mode = 0;
    int fd = 0;
    char input[16];
    pthread_t th;

    // pthread_create(&th, NULL, (void*)&fun, NULL);

    while(1)
    {
        memset(input, 0, sizeof(input));
        if(scanf("%s", input) > 0)
        {
            //wav 播放
            if(mode == 0)
            {
                if(input[0] == '1')
                    wmix_play_wav("./test.wav");
                else if(input[0] == '2')
                    wmix_play_wav("./test2.wav");
                else if(input[0] == '3')
                    wmix_play_wav("./test3.wav");
                else if(input[0] == '0')
                    wmix_play_wav("./capture.wav");

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play_wav("./music.wav");
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play_wav("./music2.wav");
            }
            else
            {
                if(input[0] == '1')
                    wmix_play_wav2("./test.wav");
                else if(input[0] == '2')
                    wmix_play_wav2("./test2.wav");
                else if(input[0] == '3')
                    wmix_play_wav2("./test3.wav");
                else if(input[0] == '0')
                    wmix_play_wav2("./capture.wav");

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play_wav2("./music.wav");
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play_wav2("./music2.wav");

                else if(input[0] == 'c')
                    wmix_play_wav2(NULL);
            }
            
            //数据流 播放
            if(input[0] == 't' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun, NULL);
            else if(input[0] == 't' && input[1] == '1')
                fd = wmix_stream_open(2, 16, 44100);
            else if(input[0] == 't' && input[1] == '2')
            {
                close(fd);
                fd = 0;
            }

            //设置音量
            else if(input[0] == 'v' && input[1] == '1' && input[2] == '0')
                wmix_set_volume(10, 10);
            else if(input[0] == 'v')
                wmix_set_volume(input[1] - '0', 10);

            //模式切换
            else if(input[0] == 's')
                mode = !mode;

            //退出
            else if(input[0] == 'q')
                break;
        }
        usleep(10000);
    }
    //
    return 0;
}
