#ifndef _PLAY_H_
#define _PLAY_H_

#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

//默认录音参数
#define DEFAULT_CHANNELS         (2)   //通道数(channel)：该参数为1表示单声道，2则是立体声。
#define DEFAULT_SAMPLE_RATE      (8000)//采样率(rate)：每秒钟采样次数，该次数是针对桢而言
#define DEFAULT_SAMPLE_LENGTH    (16)  //样本长度(sample)：样本是记录音频数据最基本的单位，常见的有8位和16位。

/*******************sndwav_common*******************************************************************/
typedef long long off64_t;

typedef struct SNDPCMContainer {
    snd_pcm_t *handle;
    snd_output_t *log;
    snd_pcm_uframes_t chunk_size;
    snd_pcm_uframes_t buffer_size;//缓冲区大小
    snd_pcm_format_t format;
    uint16_t channels;
    size_t chunk_bytes;//每次读取到缓冲区的字节数
    size_t bits_per_sample;
    size_t bits_per_frame;

    uint8_t *data_buf;
} SNDPCMContainer_t;

//-------------------- XXX --------------------------
#include <pthread.h>

#define DEFAULT_CIRCLE_CHANNELS 2
#define DEFAULT_CIRCLE_SAMPLE 16
#define DEFAULT_CIRCLE_FREQ 44100
//循环缓冲区大小
#define DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE 524288//512K //1048576//1M
//载入wav时的缓冲区大小
#define DEFAULT_WAV_CACHE_BUFF_SIZE 262400//256K //524288//512K

typedef union CircleBuffPoint
{
    int8_t *S8;
    uint8_t *U8;
    int16_t *S16;
    int16_t *U16;
    int32_t *S32;
    int32_t *U32;
}CircleBuff_Point;

typedef struct SNDPCMContainer2 {
    SNDPCMContainer_t *playback;
    //
    uint8_t *buff;//缓冲区
    CircleBuff_Point start, end;//缓冲区头尾指针
    CircleBuff_Point head, tail;//当前缓冲区读写指针
    // pthread_mutex_t lock;//互斥锁
    //
    pthread_t th_paly;//播放指针管理线程
    pthread_t th_msg;//接收消息线程
    //
    uint8_t run;//全局正常运行标志
    uint32_t tick;//播放指针启动至今走过的字节数
} SNDPCMContainer2_t;

///////
int play_wav(char *filename);

int record_wav(char *filename,uint32_t duration_time);

int sys_volume_set(uint8_t vol_value);

///////

SNDPCMContainer2_t *circle_play_init(void);
void circle_play_exit(SNDPCMContainer2_t *playback2);

CircleBuff_Point circle_play_load_wavStream(
    SNDPCMContainer2_t *playback2,
    CircleBuff_Point src,
    uint32_t srcU8Len,
    uint32_t freq,
    uint8_t channels,
    uint8_t sample,
    CircleBuff_Point head);

void circle_play_load_wav(
    SNDPCMContainer2_t *playback2,
    char *wavPath);

#endif

