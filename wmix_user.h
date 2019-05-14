#ifndef _WMIX_USER_H_
#define _WMIX_USER_H_

#include <stdint.h>

//设置音量 count/div 例如: 30% -> 30/100
//count: 音量
//div: 分度
void wmix_set_volume(uint8_t count, uint8_t div);

//播放 wav 文件
void wmix_play_wav(char *wavPath);

typedef struct{
    int fd_write;
    int msg_fd;
}WMix_Stream;

//成功返回 失败返回 NULL
WMix_Stream *wmix_stream_init(
    uint8_t channels,
    uint8_t sample,
    uint16_t freq);
//返回发送字节数
int wmix_stream_transfer(WMix_Stream *stream, uint8_t *data, int len);
//关闭 fd
void wmix_stream_release(WMix_Stream *stream);

#endif
