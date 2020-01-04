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

void fun_write_fifo(void)
{
    int fd;
    ssize_t ret, total = 0;
    uint8_t buff[4096];
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

void fun_read_fifo(void)
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

void fun_wr_fifo(void)
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

// int main(void)
// {
//     int fd;
//     ssize_t ret, total = 0;
//     uint8_t buff[4096];
//     int stream = wmix_stream_open(1, 16, 16000, 0);
//     if(stream > 0)
//     {
//         // fd = open("./music.wav", O_RDONLY);
//         fd = open("./test.wav", O_RDONLY);
//         if(fd > 0)
//         {
//             //跳过文件头
//             read(fd, buff, 44);
//             //
//             while(1)
//             {
//                 ret = read(fd, buff, 600);
//                 if(ret > 0)
//                     write(stream, buff, ret);
//                 else
//                     break;
//                 total += ret;
//             }
//             //
//             close(fd);
//         }
//         //
//         close(stream);
//         //
//         printf("wav write end: %ld\n", (long)total);
//     }
//     return 0;
// }

int main(void)
{
    char input[16];
    pthread_t th;

    // pthread_create(&th, NULL, (void*)&fun, NULL);

    while(1)
    {
        memset(input, 0, sizeof(input));
        if(scanf("%s", input) > 0)
        {
            //数据流 播放
            if(input[0] == 'b' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun_read_fifo, NULL);
            else if(input[0] == 'n' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun_write_fifo, NULL);
            else if(input[0] == 'm' && input[1] == 0)
                pthread_create(&th, NULL, (void*)&fun_wr_fifo, NULL);

            //数据流 播放
            if(input[0] == 'i' && input[1] == 0)
                wmix_play("./capture.wav", 0, 0, 0);
            else if(input[0] == 'o' && input[1] == 0)
                wmix_play("./capture.wav", 0, 0, 0);
            else if(input[0] == 'p' && input[1] == 0)
                wmix_play("./capture.aac", 0, 0, 0);

            //录音
            else if(input[0] == 'j')
                pthread_create(&th, NULL, (void*)&fun_read_fifo, NULL);
            else if(input[0] == 'k')
                wmix_record("./capture.wav", 1, 16, 8000, 5, false);//录至文件
            else if(input[0] == 'l')
                wmix_record("./capture.aac", 1, 16, 8000, 5, false);//录至文件

            //设置音量
            else if(input[0] == 'v' && input[1] == '1' && input[2] == '0')
                wmix_set_volume(10, 10);
            else if(input[0] == 'v')
                wmix_set_volume(input[1] - '0', 10);

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
