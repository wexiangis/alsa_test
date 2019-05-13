#ifndef _WMIX_USER_H_
#define _WMIX_USER_H_

#include <stdint.h>

//设置音量 count/div 例如: 30% -> 30/100
//count: 音量
//div: 分度
void wmix_set_volume(uint8_t count, uint8_t div);

void wmix_play_wav(char *wavPath);

void wmix_play_stream(
    uint8_t *stream,
    uint32_t len,
    uint16_t freq,
    uint8_t channels,
    uint8_t sample);

#endif
