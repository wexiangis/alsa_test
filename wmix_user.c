#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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

void wmix_play_stream(
    uint8_t *stream,
    uint32_t len,
    uint16_t freq,
    uint8_t channels,
    uint8_t sample)
{
    WMix_Msg msg;
    //msg初始化
    MSG_INIT();
    //装填 message
    memset(&msg, 0, sizeof(WMix_Msg));
    msg.type = (channels<<24) | (sample<<16) | freq;
    //发出
    msgsnd(msg_fd, &msg, sizeof(WMix_Msg), IPC_NOWAIT);
}