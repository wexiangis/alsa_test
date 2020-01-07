
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "rtp.h"
#include "g711codec.h"

#define RTP_IP "127.0.0.1"
#define RTP_PORT 9832//9832

int main(int argc, char* argv[])
{
    int fd;
    int ret;
    SocketStruct *ss;
    RtpPacket rtpPacket;
    char wsdp = 0;

    int readSize = RTP_PCMA_PKT_SIZE;
    int chn = 2, freq = 8000;
    int timestamp = 1000000*1024/(16000*2*2);
    int seekStart = 0;

    __time_t tick1, tick2;

    unsigned char pcm[RTP_PCMA_PKT_SIZE*2];

    if(argc != 4)
    {
        printf("Usage: %s <dest chn freq>\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if(fd < 0)
    {
        printf("failed to open %s\n", argv[1]);
        return -1;
    }

    if(strstr(argv[1], ".wav"))
    {
        seekStart = 44;
        read(fd, rtpPacket.payload, seekStart);
    }
    chn = atoi(argv[2]);
    freq = atoi(argv[3]);
    readSize = RTP_PCMA_PKT_SIZE*2;//chn*2*readSize;
    timestamp = 160;//20000;//1000000*readSize/(freq*chn*2);

    ss = rtp_socket(RTP_IP, RTP_PORT, 1);
    if(!ss)
    {
        printf("failed to create udp socket\n");
        close(fd);
        return -1;
    }

    rtp_header(&rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_PCMA, 0, 0, 0, 0);

    while(1)
    {
        tick1 = getTickUs();

        printf("--------------------------------\n");

        if(!wsdp)
        {
            wsdp = 1;
            rtp_create_sdp("./test.sdp", RTP_IP, RTP_PORT, chn, freq, RTP_PAYLOAD_TYPE_PCMA);
        }

        ret = read(fd, pcm, sizeof(pcm));
        if(ret < sizeof(pcm))
        {
            lseek(fd, seekStart, SEEK_SET);
            continue;
        }

        ret = PCM2G711a(pcm, rtpPacket.payload, ret, 0);

        ret = rtp_send(ss, &rtpPacket, ret);
        if(ret > 0)
            printf("send: %d, %d\n", ret, rtpPacket.rtpHeader.seq);
        
        rtpPacket.rtpHeader.timestamp += timestamp;

        tick2 = getTickUs();
        if(tick2 > tick1 && tick2 - tick1 < 20000)
            usleep(20000 - (tick2 - tick1));
        else
            usleep(1000);
        printf("tick1: %d, tick2: %d, delay: %d\n", tick1, tick2, 20000-(tick2-tick1));
    }

    close(fd);
    close(ss->fd);
    free(ss);

    return 0;
}
