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

///////
int play_wav(char *filename);
int record_wav(char *filename,uint32_t duration_time);

int sys_volume_set(uint8_t vol_value);

#endif

