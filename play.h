#ifndef _PLAY_H_
#define _PLAY_H_

#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

//默认录音参数
#define DEFAULT_CHANNELS         (2)   //通道数(channel)：该参数为1表示单声道，2则是立体声。
#define DEFAULT_SAMPLE_RATE      (8000)//采样率(rate)：每秒钟采样次数，该次数是针对桢而言
#define DEFAULT_SAMPLE_LENGTH    (16)  //样本长度(sample)：样本是记录音频数据最基本的单位，常见的有8位和16位。
#define DEFAULT_DURATION_TIME    (10)  //录音时间 单位：秒

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
#define DEFAULT_CIRCLE_FREQ 44100
#define DEFAULT_CIRCLE_SAMPLE 16
#define DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE 1048576//262144
#define DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE16 (DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE/2)
#define DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE32 (DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE/4)

typedef union CircleBuffType
{
    uint8_t U8[DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE];
    uint16_t U16[DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE16];
    uint32_t U32[DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE32];
}CircleBuff_Type;

typedef union CircleBuffPoint
{
    uint8_t *U8;
    int16_t *S16;
    int32_t *S32;
}CircleBuff_Point;

typedef struct SNDPCMContainer2 {
    SNDPCMContainer_t *playback;
    //
    CircleBuff_Type buff;
    CircleBuff_Point start, end;//缓冲区头尾指针
    CircleBuff_Point head, tail;//当前缓冲区读写指针
    pthread_mutex_t lock;
    //
    pthread_t th_paly;
    pthread_t th_msg;
    //
    uint8_t run;
} SNDPCMContainer2_t;

///////
int play_wav(char *filename);
int record_wav(char *filename,uint32_t duration_time);
int sys_volume_set(uint8_t vol_value);

///////
SNDPCMContainer2_t *circle_play_init(void);
void circle_play_exit(SNDPCMContainer2_t *playback2);

void circle_play_load_wav(SNDPCMContainer2_t *playback2, char *wavPath);
CircleBuff_Point circle_play_load_wavStream(SNDPCMContainer2_t *playback2, 
    CircleBuff_Point src, uint32_t len, 
    uint32_t freq, uint8_t channels, uint8_t sample, CircleBuff_Point head);

#endif

