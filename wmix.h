#ifndef _WMIX_H_
#define _WMIX_H_

#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

//默认录音参数
#define DEFAULT_CHANNELS         (2)   //通道数(channel)：该参数为1表示单声道，2则是立体声。
#define DEFAULT_SAMPLE_RATE      (44100)//采样率(rate)：每秒钟采样次数，该次数是针对桢而言
#define DEFAULT_SAMPLE_LENGTH    (16)  //样本长度(sample)：样本是记录音频数据最基本的单位，常见的有8位和16位。

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

//-------------------- Wav Mix --------------------------

#include <pthread.h>
#include <sys/ipc.h>

#define WMIX_MSG_PATH "/tmp/wmix"
#define WMIX_MSG_PATH_CLEAR "rm -rf /tmp/wmix/*"
#define WMIX_MSG_PATH_AUTHORITY "chmod 777 /tmp/wmix -R"
#define WMIX_MSG_ID   'w'
#define WMIX_MSG_BUFF_SIZE 128

typedef struct{
    long type;// 1/设置音量 2/播放wav文件 3/stream 4/互斥播放 5/复位
    uint8_t value[WMIX_MSG_BUFF_SIZE];
}WMix_Msg;

#define WMIX_CHANNELS 2
#define WMIX_SAMPLE 16
#define WMIX_FREQ 44100
//循环缓冲区大小
#define WMIX_BUFF_SIZE 524288//512K //1048576//1M

typedef union
{
    int8_t *S8;
    uint8_t *U8;
    int16_t *S16;
    int16_t *U16;
    int32_t *S32;
    int32_t *U32;
}WMix_Point;

typedef struct{
    SNDPCMContainer_t *playback;
    //
    uint8_t *buff;//缓冲区
    WMix_Point start, end;//缓冲区头尾指针
    WMix_Point head, tail;//当前缓冲区读写指针
    // pthread_mutex_t lock;//互斥锁
    //
    uint8_t run;//全局正常运行标志
    uint32_t tick;//播放指针启动至今走过的字节数
    uint32_t thread_count;//线程计数 增加线程时+1 减少时-1 等于0时全部退出
    uint32_t thread_tick;
    //
    key_t msg_key;
    int msg_fd;
    //
    uint8_t reduceMode;
}WMix_Struct;

//设置音量
long sys_volume_set(uint8_t count, uint8_t div);

//-------------------- 独占方式播放 --------------------

//播放
int play_wav(char *filename);
//录音
int record_wav(char *filename,uint32_t duration_time);

//-------------------- 混音方式播放 --------------------

//-- 支持混音范围: 44100Hz及以下频率,采样为16bit的音频 --

//初始化
WMix_Struct *wmix_init(void);

//关闭
void wmix_exit(WMix_Struct *wmix);

//载入音频数据 的方式播放
WMix_Point wmix_load_wavStream(
    WMix_Struct *wmix,
    WMix_Point src,
    uint32_t srcU8Len,
    uint16_t freq,
    uint8_t channels,
    uint8_t sample,
    WMix_Point head,
    uint8_t reduce);

//指定wav文件 的方式播放
void wmix_load_wav(
    WMix_Struct *wmix,
    char *wavPath,
    char *msgPath,
    uint8_t reduce,
    uint8_t repeatInterval);

//指定mp3文件 的方式播放
void wmix_load_mp3(
    WMix_Struct *wmix,
    char *mp3Path,
    char *msgPath,
    uint8_t reduce,
    uint8_t repeatInterval);

#endif

