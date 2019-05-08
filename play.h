#ifndef _PLAY_H_
#define _PLAY_H_

#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

/*******************sndwav_common*******************************************************************/
typedef long long off64_t;

typedef struct SNDPCMContainer {
    snd_pcm_t *handle;
    snd_output_t *log;
    snd_pcm_uframes_t chunk_size;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_format_t format;
    uint16_t channels;
    size_t chunk_bytes;
    size_t bits_per_sample;
    size_t bits_per_frame;

    uint8_t *data_buf;
} SNDPCMContainer_t;

///////
int play_wav(char *filename);
int sys_volume_set(uint8_t vol_value);

//////
int record_wav(char *filename,uint32_t duration_time);

#endif

