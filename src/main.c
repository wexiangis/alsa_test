#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wmix_user.h"
#include "wav.h"

void fun(void)
{
    int fd;
    ssize_t ret, total = 0;
    uint8_t buff[4096];
    // int stream = wmix_stream_open(2, 16, 44100);
    int stream = wmix_stream_open(1, 16, 22050, 0);
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

void fun2(void)
{
    uint8_t chn = 2;
    uint8_t sample = 16;
    uint16_t freq = 16000;
    uint16_t second = 5;
    //
    WAVContainer_t wav;
    char buff[1024];
    size_t ret, total = 0;
    int fd;
    //
    int fd_record = wmix_record_stream_open(chn, sample, freq);
    size_t sum = chn*sample/8*freq*second;

    if(fd_record > 0)
    {
        fd = open("./capture.wav", O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if(fd <= 0)
        {
            close(fd_record);
            return;
        }
        //
        WAV_Params(&wav, second, chn, sample, freq);
        WAV_WriteHeader(fd, &wav);
        //
        while(1)
        {
            ret = read(fd_record, buff, sizeof(buff));
            if(ret > 0)
            {
                if(write(fd, buff, ret) < 1)
                    break;
                total += ret;
                if(total >= sum)
                    break;
            }
            else
                break;
        }
        //
        close(fd_record);
        close(fd);
        printf("wav write end: %ld\n", total);
    }
}

void fun3(void)
{
    uint8_t chn = 2;
    uint8_t sample = 16;
    uint16_t freq = 16000;
    uint16_t second = 10;
    //
    char buff[1024];
    size_t ret, total = 0;
    //
    int fd = wmix_stream_open(chn, sample, freq, 3);
    int fd_record = wmix_record_stream_open(chn, sample, freq);
    size_t sum = chn*sample/8*freq*second;

    if(fd_record > 0 && fd > 0)
    {
        while(1)
        {
            ret = read(fd_record, buff, sizeof(buff));
            if(ret > 0)
            {
                if(write(fd, buff, ret) < 1)
                    break;
                total += ret;
                if(total >= sum)
                    break;
            }
            else
                break;
        }
        //
        close(fd_record);
        close(fd);
        printf("wav write end: %ld\n", total);
    }
}

int main()
{
    int mode = 0;
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
                    wmix_play("./test.wav", 0);
                else if(input[0] == '2')
                    wmix_play("./test2.wav", 0);
                else if(input[0] == '0')
                    wmix_play("./capture.wav", 0);

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play("./music.wav", 0);
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play("./music2.wav", 0);
                else if(input[0] == 'm' && input[1] == '3')
                    wmix_play("./music3.mp3", 0);
                else if(input[0] == 'm' && input[1] == '4')
                    wmix_play("./music4.mp3", 0);
                else if(input[0] == 'm' && input[1] == '5')
                    wmix_play("./music5.mp3", 0);
            }
            else if(mode == 1)
            {
                if(input[0] == '1')
                    wmix_play2("./test.wav", 0, 0);
                else if(input[0] == '2')
                    wmix_play2("./test2.wav", 0, 0);
                else if(input[0] == '0')
                    wmix_play2("./capture.wav", 0, 0);

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play2("./music.wav", 0, 0);
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play2("./music2.wav", 0, 0);
                else if(input[0] == 'm' && input[1] == '3')
                    wmix_play2("./music3.mp3", 0, 0);
                else if(input[0] == 'm' && input[1] == '4')
                    wmix_play2("./music4.mp3", 0, 0);
                else if(input[0] == 'm' && input[1] == '5')
                    wmix_play2("./music5.mp3", 0, 0);

                else if(input[0] == 'c')
                    wmix_play2(NULL, 0, 0);
            }
            else if(mode == 2)
            {
                if(input[0] == '1')
                    wmix_play("./test.wav", 4);
                else if(input[0] == '2')
                    wmix_play("./test2.wav", 4);
                else if(input[0] == '0')
                    wmix_play("./capture.wav", 4);

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play("./music.wav", 4);
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play("./music2.wav", 4);
                else if(input[0] == 'm' && input[1] == '3')
                    wmix_play("./music3.mp3", 4);
                else if(input[0] == 'm' && input[1] == '4')
                    wmix_play("./music4.mp3", 4);
                else if(input[0] == 'm' && input[1] == '5')
                    wmix_play("./music5.mp3", 4);
            }
            else if(mode == 3)
            {
                if(input[0] == '1')
                    wmix_play2("./test.wav", 0, 3);
                else if(input[0] == '2')
                    wmix_play2("./test2.wav", 0, 3);
                else if(input[0] == '0')
                    wmix_play2("./capture.wav", 0, 3);

                else if(input[0] == 'm' && input[1] == '1')
                    wmix_play2("./music.wav", 0, 3);
                else if(input[0] == 'm' && input[1] == '2')
                    wmix_play2("./music2.wav", 0, 3);
                else if(input[0] == 'm' && input[1] == '3')
                    wmix_play2("./music3.mp3", 0, 3);
                else if(input[0] == 'm' && input[1] == '4')
                    wmix_play2("./music4.mp3", 0, 3);
                else if(input[0] == 'm' && input[1] == '5')
                    wmix_play2("./music5.mp3", 0, 3);

                else if(input[0] == 'c')
                    wmix_play2(NULL, 0, 0);
            }
            
            //数据流 播放
            if(input[0] == 't' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun, NULL);

            //录音
            else if(input[0] == 'j')
                pthread_create(&th, NULL, (void*)&fun2, NULL);//获取数据流
            else if(input[0] == 'k')
                wmix_record("./capture.wav", 1, 16, 8000, 5);//录至文件
            // else if(input[0] == 'l')
            //     pthread_create(&th, NULL, (void*)&fun3, NULL);//获取数据流

            //设置音量
            else if(input[0] == 'v' && input[1] == '1' && input[2] == '0')
                wmix_set_volume(10, 10);
            else if(input[0] == 'v')
                wmix_set_volume(input[1] - '0', 10);

            //模式切换
            else if(input[0] == 's')
            {
                mode += 1;
                if(mode > 3)
                    mode = 0;
                printf("mode : %d  %s\n", mode, 
                    mode==0?"普通播放模式":(
                        mode==1?"互斥模式":(
                            mode==2?"背景reduce模式":"互斥循环播放模式"
                        )
                    ));
            }

            //复位
            else if(input[0] == 'r')
                wmix_reset();

            //退出
            else if(input[0] == 'q')
                break;
        }
        usleep(10000);
    }
    //
    return 0;
}
