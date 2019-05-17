#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "wmix_user.h"

#define WMIX_MSG_PATH "/tmp/wmix"
#define WMIX_MSG_ID   'w'
#define WMIX_MSG_BUFF_SIZE 128

typedef struct{
    long type;// 1/设置音量 2/播放wav文件 3/stream 4/互斥播放 5/复位
    uint8_t value[WMIX_MSG_BUFF_SIZE];
}WMix_Msg;

#define MSG_INIT() \
key_t msg_key;\
int msg_fd;\
if((msg_key = ftok(WMIX_MSG_PATH, WMIX_MSG_ID)) == -1){\
    fprintf(stderr, "wmix_play_wav: ftok err\n");\
    return;\
}if((msg_fd = msgget(msg_key, 0666)) == -1){\
    fprintf(stderr, "wmix_play_wav: msgget err\n");\
    return;\
}

void wmix_reset(void)
{
    WMix_Msg msg;
    //msg初始化
    MSG_INIT();
    //装填 message
    msg.type = 5;
    //发出
    msgsnd(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, IPC_NOWAIT);
}

void wmix_set_volume(uint8_t count, uint8_t div)
{
    WMix_Msg msg;
    //msg初始化
    MSG_INIT();
    //装填 message
    memset(&msg, 0, sizeof(WMix_Msg));
    msg.type = 1;
    if(count > div)
        msg.value[0] = div;
    else
        msg.value[0] = count;
    msg.value[1] = div;
    //发出
    msgsnd(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, IPC_NOWAIT);
}

void wmix_play_wav(char *wavPath)
{
    if(!wavPath)
        return;
    WMix_Msg msg;
    //msg初始化
    MSG_INIT();
    //装填 message
    memset(&msg, 0, sizeof(WMix_Msg));
    msg.type = 2;
    if(strlen(wavPath) > WMIX_MSG_BUFF_SIZE){
        fprintf(stderr, "wmix_play_wav: wavPath > max len (%d)\n", WMIX_MSG_BUFF_SIZE);
        return ;
    }
    strcpy((char*)msg.value, wavPath);
    //发出
    msgsnd(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, IPC_NOWAIT);
}

//自动命名: 主路径WMIX_MSG_PATH + fifo + pid + 0~255
char *wmix_auto_path(char *buff, int pid, uint8_t id)
{
    sprintf(buff, "%s/fifo-%05d%03d", WMIX_MSG_PATH, pid, id);
    return buff;
}

//自动命名: 主路径WMIX_MSG_PATH + wav + pid + 0~255
char *wmix_auto_path2(char *buff, int pid, uint8_t id)
{
    sprintf(buff, "%s/wav-%05d%03d", WMIX_MSG_PATH, pid, id);
    return buff;
}

void wmix_play_wav2(char *wavPath)
{
    static uint8_t id_w = 0;
    uint8_t id_f, id_max = 5;// id_max 用于提高容错率,防止打断失败
    char msgPath[128] = {0};
    WMix_Msg msg;
    key_t msg_key;
    int msg_fd;
    int timeout;
    //
    for(id_f = 0; id_f < id_max; id_f++)
    {
        wmix_auto_path2(msgPath, getpid(), id_f);
        //关闭旧的播放线程
        if(access(msgPath, F_OK) == 0)
        {
            if((msg_key = ftok(msgPath, WMIX_MSG_ID)) == -1){
                fprintf(stderr, "wmix_stream_init: ftok err\n");
                return;
            }
            if((msg_fd = msgget(msg_key, 0666)) == -1){
                // fprintf(stderr, "wmix_stream_init: msgget err\n");
                remove(msgPath);
                continue;
            }
            //通知关闭
            msgctl(msg_fd, IPC_RMID, NULL);
            //等待关闭
            timeout = 20;//200ms超时
            do{
                if(timeout-- == 0)
                    break;
                usleep(10000);
            }while(access(msgPath, F_OK) == 0);
            //
            remove(msgPath);
        }
    }
    //wavPath == NULL 时关闭播放
    if(wavPath)
    {
        wmix_auto_path2(msgPath, getpid(), id_w++);
        if(id_w >= id_max)
            id_w = 0;
        //
        if(strlen(msgPath) + strlen(wavPath) + 2 > WMIX_MSG_BUFF_SIZE){
            fprintf(stderr, "wmix_play_wav: wavPath > max len (%ld)\n", (long)(WMIX_MSG_BUFF_SIZE-strlen(msgPath)-2));
            return;
        }
        //
        if((msg_key = ftok(WMIX_MSG_PATH, WMIX_MSG_ID)) == -1){
            fprintf(stderr, "wmix_stream_init: ftok err\n");
            return;
        }
        if((msg_fd = msgget(msg_key, 0666)) == -1){
            fprintf(stderr, "wmix_stream_init: msgget err\n");
            return;
        }
        //装填 message
        memset(&msg, 0, sizeof(WMix_Msg));
        msg.type = 4;
        //wav路径 + msg路径 
        strcpy((char*)msg.value, wavPath);
        strcpy((char*)&msg.value[strlen(wavPath)+1], msgPath);
        //发出
        msgsnd(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, IPC_NOWAIT);
        //等待启动
        timeout = 20;//200ms超时
        do{
            if(timeout-- == 0)
                break;
            usleep(10000);
        }while(access(msgPath, F_OK) != 0);
    }
}

int wmix_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq)
{
    static uint8_t id = 0;
    //
    if(!freq || !channels || !sample)
        return 0;
    //
    int fd;
    int timeout;
    char *path;
    WMix_Msg msg;
    //msg初始化
    key_t msg_key;
    int msg_fd;
    //
    if((msg_key = ftok(WMIX_MSG_PATH, WMIX_MSG_ID)) == -1){
        fprintf(stderr, "wmix_stream_init: ftok err\n");
        return 0;
    }
    if((msg_fd = msgget(msg_key, 0666)) == -1){
        fprintf(stderr, "wmix_stream_init: msgget err\n");
        return 0;
    }
    //路径创建
    memset(&msg, 0, sizeof(WMix_Msg));
    path = wmix_auto_path((char*)&msg.value[4], getpid(), id++);
    // remove(path);
    //装填 message
    msg.type = 3;
    msg.value[0] = channels;
    msg.value[1] = sample;
    msg.value[2] = (freq>>8)&0xFF;
    msg.value[3] = freq&0xFF;
    //发出
    msgsnd(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, IPC_NOWAIT);
    //等待路径创建
    timeout = 10;//100ms超时
    do{
        if(timeout-- == 0)
            break;
        usleep(10000);
    }while(access(path, F_OK) != 0);
    //
    if(access(path, F_OK) != 0){
        fprintf(stderr, "wmix_stream_init: %s timeout\n", path);
        return 0;
    }
    //
    if(fork() == 0)
        open(path, O_RDONLY | O_NONBLOCK);//防止下面的写阻塞打不开
    else
        fd = open(path, O_WRONLY);
    //
    return fd;
}
