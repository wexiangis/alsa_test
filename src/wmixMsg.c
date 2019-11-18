
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "wmix_user.h"

void help(char *argv0)
{
    printf(
        "\n"
        "Usage: %s [option] WavOrMp3File\n"
        "\n"
        "Opition:\n"
        "  -l : 排队模式,排到最后一位(默认模式)\n"
        "  -i : 排队模式,排到第一位\n"
        "  -m : 混音模式(不设置时为排队模式)\n"
        "  -b : 打断模式(不设置时为排队模式)\n"
        "  -t interval : 循环播放模式,间隔秒,取值[1~255]\n"
        "  -d reduce : 背景音削减倍数,取值[1~255]\n"
        "  -v volume : 音量设置0~10\n"
        "  -k id : 关闭指定id的语音,id=0时关闭所有\n"
        "  -r : 录音模式(默认单通道/16bits/44100Hz/5秒)\n"
        "  -rc chn : 指定录音通道数[1,2]\n"
        "  -rr freq : 指定录音频率[8000,11025,16000,22050,32000,44100]\n"
        "  -rt time : 指定录音时长秒\n"
        "  -? --help : 显示帮助\n"
        "\n"
        "其它说明:\n"
        "  文件支持格式 : wav,mp3\n"
        "  返回 : 0/正常 <0/错误 >0/语音id,用于\"-k\"关闭\n"
        "\n"
        "软件版本: %s\n"
        "\n"
        "Example:\n"
        "  %s -v 10\n"
        "  %s ./music.wav\n"
        "  %s ./music.wav -t 1\n"
        "  %s ./music.wav -r ./record.wav\n"
        "\n"
        ,argv0, WMIX_VERSION, argv0, argv0, argv0, argv0);
}

int main(int argc, char **argv)
{
    int i;
    bool record = false;//播音模式
    int interval = 0;
    int reduce = 0;
    int volume = -1;
    int id = -1;
    int order = 2;
    int rt = 5, rc = 1, rr = 44100;

    char *filePath = NULL;
    char tmpPath[128] = {0};
    char tmpPath2[128] = {0};

    if(argc < 2)
    {
        help(argv[0]);
        return 0;
    }

    for(i = 1; i < argc; i++)
    {
        if(strlen(argv[i]) == 2 && strstr(argv[i], "-r"))
        {
            record = true;
        }
        else if(strlen(argv[i]) == 3 && strstr(argv[i], "-rt") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &rt);
        }
        else if(strlen(argv[i]) == 3 && strstr(argv[i], "-rc") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &rc);
        }
        else if(strlen(argv[i]) == 3 && strstr(argv[i], "-rr") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &rr);
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-b"))
        {
            order = -1;
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-m"))
        {
            order = 0;
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-i"))
        {
            order = 1;
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-l"))
        {
            order = 2;
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-t") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &interval);
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-d") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &reduce);
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-v") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &volume);
        }
        else if(strlen(argv[i]) == 2 && strstr(argv[i], "-k") && i+1 < argc)
        {
            i += 1;
            sscanf(argv[i], "%d", &id);
        }
        else if(strstr(argv[i], "-?") || strstr(argv[i], "-help"))
        {
            help(argv[0]);
            return 0;
        }
        else
        {
            filePath = argv[i];
        }
    }

    if(volume >= 0 && volume < 11)
    {
        wmix_set_volume(volume, 10);
        if(filePath == NULL)
            return 0;
    }

    if(id >= 0)
    {
        wmix_play_kill(id);
        if(filePath == NULL)
            return 0;
    }
    id = 0;

    if(filePath == NULL)
    {
        printf("\nparam err !!\n");
        help(argv[0]);
        return -1;
    }

    if(filePath && filePath[0] == '.')
    {
        if(getcwd(tmpPath, sizeof(tmpPath)))
        {
            snprintf(tmpPath2, sizeof(tmpPath2), "%s/%s", tmpPath, filePath);
            filePath = tmpPath2;
            // printf("%s\n", filePath);
        }
    }

    if(record)
        wmix_record(filePath, rc, 16, rr, rt);
    else
    {
        id = wmix_play(filePath, reduce, interval, order);
        if(id > 0)
            printf("id: %d\n", id);
    }

    return id;
}
