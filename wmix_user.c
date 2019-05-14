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

#define WMIX_MSG_PATH "/var/tmp/wmix"
#define WMIX_MSG_ID   'w'
#define WMIX_MSG_BUFF_SIZE 128

typedef struct{
    long type;// 1/设置音量 2/播放wav文件 3/stream
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
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
}

void wmix_play_wav(char *wavPath)
{
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
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
}

char *wmix_auto_path(char *buff, int pid, uint8_t id)
{
    sprintf(buff, "%s/%05d-%03d", WMIX_MSG_PATH, pid, id);
    return buff;
}

WMix_Stream *wmix_stream_init(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq)
{
    static uint8_t id = 0;
    //
    if(!freq || !channels || !sample)
        return NULL;
    //
    int fd_read;
    char *path;
    WMix_Msg msg;
    WMix_Stream *stream = NULL;
    //msg初始化
    key_t msg_key;
    int msg_fd;
    // key_t msg_key_s;
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
    path = wmix_auto_path((char*)&msg.value[8], getpid(), id++);
    //
    if (mkfifo(path, 0666) < 0 && errno != EEXIST)
    {
        printf("wmix_stream_init2: create fifo failed...\n");
        return NULL;
    }
    //
    stream = (WMix_Stream *)calloc(1, sizeof(WMix_Stream));
    //
    // if((msg_key_s = ftok(path, WMIX_MSG_ID)) == -1){
    //     fprintf(stderr, "wmix_stream_init: ftok2 err\n");
    //     return 0;
    // }
    // if((stream->msg_fd = msgget(msg_key_s, IPC_CREAT|0666)) == -1){
    //     fprintf(stderr, "wmix_stream_init: msgget2 err\n");
    //     return 0;
    // }
    //装填 message
    msg.type = 3;
    msg.value[0] = channels;
    msg.value[1] = sample;
    msg.value[2] = (freq>>8)&0xFF;
    msg.value[3] = freq&0xFF;
    //
    fd_read = open(path, O_RDONLY | O_NONBLOCK);
    stream->fd_write = open(path, O_WRONLY | O_NONBLOCK);
    //
    msg.value[4] = (fd_read>>24)&0xFF;
    msg.value[5] = (fd_read>>16)&0xFF;
    msg.value[6] = (fd_read>>8)&0xFF;
    msg.value[7] = fd_read&0xFF;
    //发出
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
    //
    printf("wmix_stream_init: path: %s fd_read: %d\n", path, fd_read);
    //
    return stream;
}

int wmix_stream_transfer(WMix_Stream *stream, uint8_t *data, int len)
{
    int ret = 0;
    //
    if(!stream || !data || len < 1)
        return 0;
    //
    ret = write(stream->fd_write, data, len);
    //
    // if(ret > 0)
    // {
    //     //装填 message
    //     WMix_Msg msg;
    //     //
    //     msg.type = ret;
    //     //发出
    //     msgsnd(stream->msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
    // }
    //
    return ret;
}

void wmix_stream_release(WMix_Stream *stream)
{
    if(stream)
    {
        close(stream->fd_write);
        // msgctl(stream->msg_fd, IPC_RMID, NULL);
        free(stream);
    }
}
