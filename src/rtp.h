/*
 * 作者：_JT_
 * 博客：https://blog.csdn.net/weixin_42462202
 */

#ifndef _RTP_H_
#define _RTP_H_
#include <stdint.h>
#include <netinet/in.h>

#define RTP_VESION              2

#define RTP_PAYLOAD_TYPE_PCMU   0
#define RTP_PAYLOAD_TYPE_GSM    3
#define RTP_PAYLOAD_TYPE_G723   4
#define RTP_PAYLOAD_TYPE_PCMA   8
#define RTP_PAYLOAD_TYPE_G722   9
#define RTP_PAYLOAD_TYPE_G728   15
#define RTP_PAYLOAD_TYPE_G729   18
#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_PCMA_PKT_SIZE       160

#define RTP_HEADER_SIZE         12
/*
 *
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */
typedef struct{
    /* byte 0 */
    uint8_t cc:4;//crc标识符个数
    uint8_t x:1;//扩展标志,为1时表示rtp报头后有1个扩展包
    uint8_t p:1;//填充标志,为1时标识数据尾部有无效填充
    uint8_t v:2;//版本号

    /* byte 1 */
    uint8_t pt:7;//有小载荷类型
    uint8_t m:1;//载荷标记,视频为1帧结束,音频为会话开始
    
    /* bytes 2,3 */
    uint16_t seq;//序列号,每帧+1,随机开始,音/视频分开
    
    /* bytes 4-7 */
    uint32_t timestamp;//时间戳,us,自增
    
    /* bytes 8-11 */
    uint32_t ssrc;//同步信号源

}RtpHeader;

typedef struct {
    RtpHeader rtpHeader;    

    // uint8_t len1;//0x00
    // uint8_t len2;//0x10
    // uint8_t len3;//(datLen>>5)&0xFF
    // uint8_t len4;//datLen&0x1F

    uint8_t payload[4096];
}RtpPacket;

typedef struct{
    int fd;
    struct sockaddr_in addr;
    size_t addrSize;
}SocketStruct;

void rtp_header(RtpPacket* rtpPacket, uint8_t cc, uint8_t x,
                    uint8_t p, uint8_t v, uint8_t pt, uint8_t m,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc);

int rtp_send(SocketStruct *ss, RtpPacket* rtpPacket, uint32_t dataSize);

int rtp_recv(SocketStruct *ss, RtpPacket* rtpPacket, uint32_t *dataSize);

SocketStruct* rtp_socket(uint8_t *ip, uint16_t port, uint8_t isServer);

//type: 97/AAC 98/PCMA
void rtp_create_sdp(uint8_t *file, uint8_t *ip, uint16_t port, uint16_t chn, uint16_t freq, uint16_t type);

//----------------- AAC -------------------

typedef struct{
    unsigned int syncword;  //12 bit 同步字 '1111 1111 1111'，说明一个ADTS帧的开始
    unsigned int id;        //1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
    unsigned int layer;     //2 bit 总是'00'
    unsigned int protectionAbsent;  //1 bit 1表示没有crc，0表示有crc
    unsigned int profile;           //2 bit 表示使用哪个级别的AAC
    unsigned int samplingFreqIndex; //4 bit 表示使用的采样频率
    unsigned int privateBit;        //1 bit
    unsigned int channelCfg; //3 bit 表示声道数
    unsigned int originalCopy;      //1 bit 
    unsigned int home;              //1 bit 

    /*下面的为改变的参数即每一帧都不同*/
    unsigned int copyrightIdentificationBit;   //1 bit
    unsigned int copyrightIdentificationStart; //1 bit
    unsigned int aacFrameLength;               //13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
    unsigned int adtsBufferFullness;           //11 bit 0x7FF 说明是码率可变的码流

    /* number_of_raw_data_blocks_in_frame
     * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
     * 所以说number_of_raw_data_blocks_in_frame == 0 
     * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
     */
    unsigned int numberOfRawDataBlockInFrame; //2 bit
}AacHeader;

extern int aac_freq[];

//返回0成功
int aac_parseHeader(uint8_t* in, AacHeader* res, uint8_t show);

//返回总长度
int aac_header(uint8_t* in, uint8_t chn, uint16_t freq, uint16_t codeRate, uint16_t datLen);

#endif //_RTP_H_
