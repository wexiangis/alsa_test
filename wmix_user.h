#ifndef _WMIX_USER_H_
#define _WMIX_USER_H_

#include <stdint.h>

//设置音量 count/div 例如: 30% -> 30/100
//count: 音量  div: 分度
void wmix_set_volume(uint8_t count, uint8_t div);

//播放 wav 文件
void wmix_play_wav(char *wavPath);

//成功返回fifo的(只写)fd  失败返回0
int wmix_stream_open(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq);

#endif
