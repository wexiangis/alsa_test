#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#define WMIX_MSG_PATH "/tmp/wmix"
#define WMIX_MSG_ID   'w'
#define WMIX_MSG_BUFF_SIZE 2048

typedef struct{
    long type;// 1/设置音量 2/播放wav文件 其它/传递fifo路径,且type=chn<<24|sample<<16|freq
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

int wmix_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq)
{
    static uint8_t id = 0;
    char *path;
    //
    if(!freq || !channels || !sample)
        return 0;
    //
    WMix_Msg msg;
    memset(&msg, 0, sizeof(WMix_Msg));
    //msg初始化
    key_t msg_key, msg_key_s;
    int msg_fd, msg_fd_s = 0;
    //
    if((msg_key = ftok(WMIX_MSG_PATH, WMIX_MSG_ID)) == -1){
        fprintf(stderr, "wmix_stream_open: ftok err\n");\
        return 0;
    }
    if((msg_fd = msgget(msg_key, 0666)) == -1){
        fprintf(stderr, "wmix_stream_open: msgget err\n");
        return 0;
    }
    //路径创建
    path = wmix_auto_path((char*)&msg.value[4], getpid(), id++);
    if(access(path, F_OK) != 0)
        mkdir(path, 0666);
    //
    if((msg_key_s = ftok(path, WMIX_MSG_ID)) == -1){
        fprintf(stderr, "wmix_stream_open: ftok2 err\n");
        return 0;
    }
    if((msg_fd_s = msgget(msg_key_s, IPC_CREAT|0666)) == -1){
        fprintf(stderr, "wmix_stream_open: msgget2 err\n");
        return 0;
    }
    //装填 message
    msg.type = 3;
    msg.value[0] = channels;
    msg.value[1] = sample;
    msg.value[2] = (freq>>8)&0xFF;
    msg.value[3] = freq&0xFF;
    //发出
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
    //
    return msg_fd_s;
}

uint16_t wmix_stream_transfer(int msg_fd, uint8_t *stream, uint16_t len)
{
    uint16_t ret = 0;
    if(!msg_fd || !stream || len < 1)
        return 0;
    //装填 message
    WMix_Msg msg;
    memset(&msg, 0, sizeof(WMix_Msg));
    //
    if(len > WMIX_MSG_BUFF_SIZE)
        ret = WMIX_MSG_BUFF_SIZE;
    else
        ret = len;
    msg.type = ret;
    memcpy(msg.value, stream, ret);
    //发出
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
    //
    return ret;
}

void wmix_stream_close(int msg_fd)
{
    if(msg_fd)
        msgctl(msg_fd, IPC_RMID, NULL);
}
