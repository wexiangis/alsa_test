#ifndef _WMIX_USER_H_
#define _WMIX_USER_H_

#include <stdint.h>

//设置音量 count/div 例如: 30% -> 30/100
//count: 音量  div: 分度
void wmix_set_volume(uint8_t count, uint8_t div);

//播放 wav 和 mp3 文件
void wmix_play(char *wavOrMp3);

//播放 wav 和 mp3 文件 (互斥播放, wavOrMp3=NULL 时强制关闭播放)
void wmix_play2(char *wavOrMp3);

//播放音频流,用于播放录音
//成功返回fd(fifo的写入端)  失败返回0
int wmix_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq);

//复位
void wmix_reset(void);

#endif
