
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "wmix_user.h"

void help(char *argv0)
{
    printf(
        "\n"
        "Usage: %s [option]\n"
        "\n"
        "Opition:\n"
        // "  -m mode : 0混音模式(默认),1互斥模式\n"
        "  -t interval : 循环播放模式,间隔秒,取值[1~255]\n"
        "  -d reduce : 背景音削减倍数,取值[1~255]\n"
        "  -v volume : 音量设置0~10\n"
        "  -k id : 关闭闭指定id的语音\n"
        "  -r : 重置播放器,关闭所有音频\n"
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
        "  %s -m 0 ./music.mp3\n"
        "\n"
        ,argv0, WMIX_VERSION, argv0, argv0);
}

int main(int argc, char **argv)
{
    int i;
    int mode = 0;
    int interval = 0;
    int reduce = 0;
    int volume = -1;
    int reset = 0;
    int id = 0;
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
            reset = 1; 
        }   
        // else if(strlen(argv[i]) == 2 && strstr(argv[i], "-m") && i+1 < argc)
        // {
        //     i += 1;
        //     sscanf(argv[i], "%d", &mode);
        // }
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

    if(id > 0)
    {
        wmix_play_exit(id);
        if(filePath == NULL)
            return 0;
        id = 0;
    }

    if(reset)
    {
        wmix_reset();
        if(filePath == NULL)
            return 0;
    }

    if(mode < 0 || mode > 3 || filePath == NULL)
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

    // if(mode == 0 && interval == 0)
    //     id = wmix_play(filePath, reduce);
    // else
        id = wmix_play2(filePath, reduce, interval);

    printf("id: %d\n", id);

    return id;
}
