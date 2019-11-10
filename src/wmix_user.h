#ifndef _WMIX_USER_H_
#define _WMIX_USER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WMIX_VERSION "V2.0 - 20191110"

//----- 设置音量 count/div 例如: 30% -> 30/100 -----
//count: 音量  div: 分度
void wmix_set_volume(uint8_t count, uint8_t div);

//----- 播放 wav 和 mp3 文件 (互斥播放, wavOrMp3=NULL 时强制关闭播放) -----
//backgroundReduce: 播放当前音频时,降低背景音量
//  0: 不启用
//  >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
//  (注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得))
//repeatInterval: 音频重复播放间隔,单位 sec
//  0: 不启用
//  >0: 播放结束后间隔 repeatInterval sec 后重播
//返回: >0 正常返回特定id,可用于"wmix_play_kill(id)"
int wmix_play(char *wavOrMp3, uint8_t backgroundReduce, uint8_t repeatInterval, bool breakall);

//根据 wmix_play 返回的id关闭启动的音频,id=0时关闭所有
void wmix_play_kill(int id);

//----- 播放音频流,用于播放录音 -----
//成功返回fd(fifo的写入端)  失败返回0
//backgroundReduce: 播放当前音频时,降低背景音量
//  0: 不启用
//  >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
//  (注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得))
int wmix_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq,
    uint8_t backgroundReduce);

//----- 录音 -----
//成功返回fd(fifo的读取端)  失败返回0
//backgroundReduce: 播放当前音频时,降低背景音量
//  0: 不启用
//  >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
//注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得)
int wmix_record_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq);

//----- 录音到文件 -----
void wmix_record(
    char *wavPath,
    uint8_t channels,
    uint8_t sample,
    uint16_t freq,
    uint16_t second);

//复位
void wmix_reset(void);

#ifdef __cplusplus
};
#endif

#endif
