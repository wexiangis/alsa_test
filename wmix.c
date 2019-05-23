
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "wmix.h"
#include "wav.h"
#include "mad.h"
#include "id3.h"

/*******************************************************************************
 * 名称: sys_volume_set
 * 功能: 扬声器音量设置
 * 参数: vol_value 设置的音量值 (范围：0-10之间)
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
long sys_volume_set(uint8_t count, uint8_t div)
{
    snd_mixer_t *mixer;
    snd_mixer_elem_t *pcm_element;
    long volume_value = (long)(count>div?div:count);
    long volume_div = (long)div;
    //
    snd_mixer_open(&mixer, 0);
    snd_mixer_attach(mixer, "default");
    snd_mixer_selem_register(mixer, NULL, NULL);
    snd_mixer_load(mixer);
    //找到Pcm对应的element
    pcm_element = snd_mixer_first_elem(mixer);/* 取得第一个 element，也就是 Master */
    snd_mixer_selem_set_playback_volume_range(pcm_element, 0, volume_div);//设置音量范围：0-100之间
    //左音量
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_LEFT,volume_value) < 0)
        return -1;
    //右音量
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_RIGHT,volume_value) < 0)
       return -1;
    //
    snd_mixer_selem_get_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_LEFT,&volume_value);//获取音量
    printf("volume: %ld / %ld\r\n", volume_value, volume_div);
    //处理事件
    snd_mixer_handle_events(mixer);
    snd_mixer_close(mixer);
    //
    return volume_value;

}

/*****************************sndwav_common**************************************************************/

/*******************************************************************************
 * 名称: SNDWAV_P_GetFormat
 * 功能: wav文件格式获取
 * 参数: wav    ： WAVContainer_t结构体指针
 *      snd_format： snd_pcm_format_t结构体指针
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int SNDWAV_P_GetFormat(WAVContainer_t *wav, snd_pcm_format_t *snd_format)
{
    if (LE_SHORT(wav->format.format) != WAV_FMT_PCM)
        return -1;

    switch (LE_SHORT(wav->format.sample_length)) {
    case 16:
        *snd_format = SND_PCM_FORMAT_S16_LE;
        break;
    case 8:
        *snd_format = SND_PCM_FORMAT_U8;
        break;
    default:
        *snd_format = SND_PCM_FORMAT_UNKNOWN;
        break;
    }

    return 0;
}
/*******************************************************************************
 * 名称: SNDWAV_ReadPcm
 * 功能: pcm设备读取
 * 参数: sndpcm ：SNDPCMContainer_t结构体指针
 *       rcount ： 读取的大小
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int SNDWAV_ReadPcm(SNDPCMContainer_t *sndpcm, size_t rcount)
{
    int r;
    size_t result = 0;
    size_t count = rcount;
    uint8_t *data = sndpcm->data_buf;

    if (count != sndpcm->chunk_size) {
        count = sndpcm->chunk_size;
    }

    while (count > 0) {
        r = snd_pcm_readi(sndpcm->handle, data, count);

        if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
            snd_pcm_wait(sndpcm->handle, 1000);
        } else if (r == -EPIPE) {
            snd_pcm_prepare(sndpcm->handle);
            fprintf(stderr, "Error: Buffer Underrun\n");
        } else if (r == -ESTRPIPE) {
            fprintf(stderr, "Error: Need suspend\n");
        } else if (r < 0) {
            fprintf(stderr, "Error: snd_pcm_writei: [%s]\n", snd_strerror(r));
            return -1;
        }

        if (r > 0) {
            result += r;
            count -= r;
            data += r * sndpcm->bits_per_frame / 8;
        }
    }
    return rcount;
}
/*******************************************************************************
 * 名称: SNDWAV_WritePcm
 * 功能: wav文件数据写入pcm设备
 * 参数: sndpcm ：SNDPCMContainer_t结构体指针
 *       wcount ： 写入的大小
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int SNDWAV_WritePcm(SNDPCMContainer_t *sndpcm, size_t wcount)
{
    int r;
    int result = 0;
    uint8_t *data = sndpcm->data_buf;

    if (wcount < sndpcm->chunk_size) {
        snd_pcm_format_set_silence(sndpcm->format,
            data + wcount * sndpcm->bits_per_frame / 8,
            (sndpcm->chunk_size - wcount) * sndpcm->channels);
        wcount = sndpcm->chunk_size;
    }
    while (wcount > 0) {
        r = snd_pcm_writei(sndpcm->handle, data, wcount);
        if (r == -EAGAIN || (r >= 0 && (size_t)r < wcount)) {
            snd_pcm_wait(sndpcm->handle, 1000);
        } else if (r == -EPIPE) {
            snd_pcm_prepare(sndpcm->handle);
            fprintf(stderr, "Error: Buffer Underrun\n");
        } else if (r == -ESTRPIPE) {
            fprintf(stderr, "Error: Need suspend\n");
        } else if (r < 0) {
            fprintf(stderr, "Error snd_pcm_writei: [%s]", snd_strerror(r));
            return -1;
        }
        if (r > 0) {
            result += r;
            wcount -= r;
            data += r * sndpcm->bits_per_frame / 8;
        }
    }
    return result;
}
/*******************************************************************************
 * 名称: SNDWAV_SetParams
 * 功能: wav文件播报参数配置
 * 参数: sndpcm ：SNDPCMContainer_t结构体指针
 *       wav    ： WAVContainer_t 结构体指针
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int SNDWAV_SetParams(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_format_t format;
    uint32_t exact_rate;
    uint32_t buffer_time, period_time;

    /* 分配snd_pcm_hw_params_t结构体  Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    /* 初始化hwparams  Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_any\n");
        return -1;
    }
    //初始化访问权限
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_access\n");
        return -1;
    }

    /* 初始化采样格式,16位 Set sample format */
    if (SNDWAV_P_GetFormat(wav, &format) < 0) {
        fprintf(stderr, "Error get_snd_pcm_format\n");
        return -1;
    }
    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_format\n");
        return -1;
    }
    sndpcm->format = format;

    /* 设置通道数量 Set number of channels */
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, LE_SHORT(wav->format.channels)) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_channels\n");
        return -1;
    }
    sndpcm->channels = LE_SHORT(wav->format.channels);
    //设置采样率，如果硬件不支持我们设置的采样率，将使用最接近的
    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    exact_rate = LE_INT(wav->format.sample_rate);
    if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near\n");
        return -1;
    }
    if (LE_INT(wav->format.sample_rate) != exact_rate) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n",
            LE_INT(wav->format.sample_rate), exact_rate);
    }

    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max\n");
        return -1;
    }
    if (buffer_time > 500000) buffer_time = 500000;
    period_time = buffer_time / 4;

    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near\n");
        return -1;
    }

    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near\n");
        return -1;
    }

    /* Set hw params */
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params(handle, params)\n");
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);
    if (sndpcm->chunk_size == sndpcm->buffer_size) {
        fprintf(stderr, "Can't use period equal to buffer size (%lu == %lu)\n", sndpcm->chunk_size, sndpcm->buffer_size);
        return -1;
    }

    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * LE_SHORT(wav->format.channels);

    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;

    // printf("---- wav info -----\n   通道数: %d\n   采样率: %d Hz\n   采样位数: %d bit\n   总数据量: %d Bytes\n"
    //     "  每次写入帧数: %d\n   每帧字节数: %ld Bytes\n   每次读写字节数: %ld Bytes\n   缓冲区大小: %d Bytes\n", 
    //     wav->format.channels,
    //     wav->format.sample_rate,
    //     wav->format.sample_length,
    //     wav->chunk.length,
    //     sndpcm->chunk_size,
    //     sndpcm->bits_per_frame/8,
    //     sndpcm->chunk_bytes,
    //     sndpcm->buffer_size);

    /* Allocate audio data buffer */
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
    if (!sndpcm->data_buf) {
        fprintf(stderr, "Error malloc: [data_buf]\n");
        return -1;
    }

    return 0;
}

/*****************************录音部分********************************************************************/

/*******************************************************************************
 * 名称: SNDWAV_PrepareWAVParams
 * 功能: wav文件录音参数配置
 * 参数: sndpcm ：SNDPCMContainer_t结构体指针
 *      duration_time ： 录音时间 单位：秒
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int SNDWAV_PrepareWAVParams(WAVContainer_t *wav,uint32_t duration_time)
{
    assert(wav);

    uint16_t channels = DEFAULT_CHANNELS;
    uint16_t sample_rate = DEFAULT_SAMPLE_RATE;
    uint16_t sample_length = DEFAULT_SAMPLE_LENGTH;
    // uint32_t duration_time = DEFAULT_DURATION_TIME;

    /* Const */
    wav->header.magic = WAV_RIFF;
    wav->header.type = WAV_WAVE;
    wav->format.magic = WAV_FMT;
    wav->format.fmt_size = LE_INT(16);
    wav->format.format = LE_SHORT(WAV_FMT_PCM);
    wav->chunk.type = WAV_DATA;

    /* User definition */
    wav->format.channels = LE_SHORT(channels);
    wav->format.sample_rate = LE_INT(sample_rate);
    wav->format.sample_length = LE_SHORT(sample_length);

    /* See format of wav file */
    wav->format.blocks_align = LE_SHORT(channels * sample_length / 8);
    wav->format.bytes_p_second = LE_INT((uint16_t)(wav->format.blocks_align) * sample_rate);

    wav->chunk.length = LE_INT(duration_time * (uint32_t)(wav->format.bytes_p_second));
    wav->header.length = LE_INT((uint32_t)(wav->chunk.length) + sizeof(wav->chunk) + sizeof(wav->format) + sizeof(wav->header) - 8);

    return 0;
}
/*******************************************************************************
 * 名称: SNDWAV_Record
 * 功能: wav文件录音
 * 参数: sndpcm ：SNDPCMContainer_t结构体指针
 *      wav ： WAVContainer_t结构体指针
 *      fd  ： 文件句柄
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
void SNDWAV_Record(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd)
{
    int64_t rest;
    size_t c, frame_size;

    if (WAV_WriteHeader(fd, wav) < 0) {
        return ;
    }

    rest = wav->chunk.length;
    while (rest > 0) {
        c = (rest <= (int64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes;
        frame_size = c * 8 / sndpcm->bits_per_frame;
        if (SNDWAV_ReadPcm(sndpcm, frame_size) != frame_size)
            break;

        if (write(fd, sndpcm->data_buf, c) != c) {
            fprintf(stderr, "Error SNDWAV_Record[write]\n");
            return ;
        }

        rest -= c;
    }
}
/*******************************************************************************
 * 名称: record_wav
 * 功能: 录音主函数
 * 参数: filename 文件路径 (如：/home/user/record.wav)
 *      duration_time 录音时间 单位：秒
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int record_wav(char *filename,uint32_t duration_time)
{
    // char *filename;
    char *devicename = "default";
    int fd;
    WAVContainer_t wav;
    SNDPCMContainer_t record;

    memset(&record, 0x0, sizeof(record));

    // filename = argv[1];
    remove(filename);
    if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {
        fprintf(stderr, "Error open: [%s]\n", filename);
        return -1;
    }

    if (snd_output_stdio_attach(&record.log, stderr, 0) < 0) {
        fprintf(stderr, "Error snd_output_stdio_attach\n");
        goto Err;
    }

    if (snd_pcm_open(&record.handle, devicename, SND_PCM_STREAM_CAPTURE, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_open [ %s]\n", devicename);
        goto Err;
    }

    if (SNDWAV_PrepareWAVParams(&wav,duration_time) < 0) {
        fprintf(stderr, "Error SNDWAV_PrepareWAVParams\n");
        goto Err;
    }

    if (SNDWAV_SetParams(&record, &wav) < 0) {
        fprintf(stderr, "Error set_snd_pcm_params\n");
        goto Err;
    }
    snd_pcm_dump(record.handle, record.log);

    SNDWAV_Record(&record, &wav, fd);

    snd_pcm_drain(record.handle);

    close(fd);
    free(record.data_buf);
    snd_output_close(record.log);
    snd_pcm_close(record.handle);
    return 0;

Err:
    close(fd);
    remove(filename);
    if (record.data_buf) free(record.data_buf);
    if (record.log) snd_output_close(record.log);
    if (record.handle) snd_pcm_close(record.handle);
    return -1;
}
/*****************************播放音频部分********************************/
/*******************************************************************************
 * 名称: SNDWAV_P_SaveRead
 * 功能: wav文件数据读取
 * 形参: count： 读取的大小
 *       buf ： 读取缓冲区
 *       fd  :  音频文件句柄
 * 返回: 无
 * 说明: 无
 ******************************************************************************/
int SNDWAV_P_SaveRead(int fd, void *buf, size_t count)
{
    int result = 0, res;

    while (count > 0)
    {
        if ((res = read(fd, buf, count)) == 0)
            break;
        if (res < 0)
            return result > 0 ? result : res;
        count -= res;
        result += res;
        buf = (char *)buf + res;
    }
    return result;
}
/*******************************************************************************
 * 名称: SNDWAV_Play
 * 功能: wav文件数据读取与写入
 * 形参: sndpcm：SNDPCMContainer_t结构体指针
 *       wav ： WAVContainer_t
 *       fd  :  音频文件句柄
 * 返回: 无
 * 说明: 无
 ******************************************************************************/
void SNDWAV_Play(SNDPCMContainer_t *sndpcm, WAVContainer_t *wav, int fd)
{
    int i = 0, bit_count = 0;
    int16_t *valueP;

    int load, ret;
    int64_t written = 0;
    int64_t c;
    int64_t count = LE_INT(wav->chunk.length);

    load = 0;
    while (written < count)
    {
        /* Must read [chunk_bytes] bytes data enough. */
        do
        {
            c = count - written;
            if (c > sndpcm->chunk_bytes)
                c = sndpcm->chunk_bytes;
            c -= load;

            if (c == 0)
                break;
            
            ret = SNDWAV_P_SaveRead(fd, sndpcm->data_buf + load, c);
            
            if (ret < 0)
            {
                fprintf(stderr, "Error safe_read\n");
                return ;
            }
            if (ret == 0)
                break;
            load += ret;
        } while ((size_t)load < sndpcm->chunk_bytes);
            
        // printf("load = %ld, written = %ld\n", load, written+load);

        if(bit_count)
        {
            for(i = 0, valueP = (int16_t*)sndpcm->data_buf; i < load/2; i++)
            {
                if(valueP[i]&0x8000)
                    valueP[i] = (0xFFFF<<(16-bit_count)) | (valueP[i]>>bit_count);
                else
                    valueP[i] = valueP[i]>>bit_count;
                // valueP[i] *= 0.0625;
            }
        }

        /* Transfer to size frame */
        load = load * 8 / sndpcm->bits_per_frame;

        ret = SNDWAV_WritePcm(sndpcm, load);

        if (ret != load)
            break;

        ret = ret * sndpcm->bits_per_frame / 8;
        written += ret;
        load = 0;
    }
}

int play_wav(char *filename)
{
    char devicename[] = "default";
    WAVContainer_t wav;//wav文件头信息
    int fd;
    SNDPCMContainer_t playback;

	memset(&playback, 0x0, sizeof(playback));

	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "Error open [%s]\n", filename);
		return -1;
	}
	//读取WAV文件头
	if (WAV_ReadHeader(fd, &wav) < 0)
	{
		fprintf(stderr, "Error WAV_Parse [%s]\n", filename);
		// goto Err;
	}
	//Creates a new output object using an existing stdio \c FILE pointer.
	if (snd_output_stdio_attach(&playback.log, stderr, 0) < 0)
	{
		fprintf(stderr, "Error snd_output_stdio_attach\n");
		goto Err;
	}
	// 打开PCM，最后一个参数为0意味着标准配置 SND_PCM_ASYNC
	if (snd_pcm_open(&playback.handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		fprintf(stderr, "Error snd_pcm_open [ %s]\n", devicename);
		goto Err;
	}
	//配置PCM参数
	if (SNDWAV_SetParams(&playback, &wav) < 0)
	{
		fprintf(stderr, "Error set_snd_pcm_params\n");
		goto Err;
	}
	snd_pcm_dump(playback.handle, playback.log);

	SNDWAV_Play(&playback, &wav, fd);

	snd_pcm_drain(playback.handle);

	close(fd);
	free(playback.data_buf);
	snd_output_close(playback.log);
	snd_pcm_close(playback.handle);
	
	return 0;

Err:
    close(fd);
    
    if (playback.data_buf)
        free(playback.data_buf);
    if (playback.log)
        snd_output_close(playback.log);
    if (playback.handle)
        snd_pcm_close(playback.handle);

    return -1;
}

//--------------------------------------- 混音方式播放 ---------------------------------------

static uint32_t get_tick_err(uint32_t current, uint32_t last)
{
    if(current > last)
        return current - last;
    else
        return last - current;
}

int SNDWAV_SetParams2(SNDPCMContainer_t *sndpcm, uint16_t freq, uint8_t channels, uint8_t sample)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_format_t format;
    uint32_t exact_rate;
    uint32_t buffer_time, period_time;

    /* 分配snd_pcm_hw_params_t结构体  Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    /* 初始化hwparams  Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(sndpcm->handle, hwparams) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_any\n");
        return -1;
    }
    //初始化访问权限
    if (snd_pcm_hw_params_set_access(sndpcm->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_access\n");
        return -1;
    }

    /* 初始化采样格式,16位 Set sample format */
    if(sample == 8)
        format = SND_PCM_FORMAT_S8;
    else if(sample == 16)
        format = SND_PCM_FORMAT_S16_LE;
    else if(sample == 24)
        format = SND_PCM_FORMAT_S24_LE;
    else if(sample == 32)
        format = SND_PCM_FORMAT_S32_LE;
    else
        return -1;
    
    if (snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_format\n");
        return -1;
    }
    sndpcm->format = format;

    /* 设置通道数量 Set number of channels */
    if (snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams, channels) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_channels\n");
        return -1;
    }
    sndpcm->channels = channels;
    //设置采样率，如果硬件不支持我们设置的采样率，将使用最接近的
    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    exact_rate = freq;
    if (snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near\n");
        return -1;
    }
    if (freq != exact_rate) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hz instead.\n",
            freq, exact_rate);
    }

    if (snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_get_buffer_time_max\n");
        return -1;
    }
    if (buffer_time > 500000) buffer_time = 500000;
    period_time = buffer_time / 4;

    if (snd_pcm_hw_params_set_buffer_time_near(sndpcm->handle, hwparams, &buffer_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_buffer_time_near\n");
        return -1;
    }

    if (snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time, 0) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near\n");
        return -1;
    }

    /* Set hw params */
    if (snd_pcm_hw_params(sndpcm->handle, hwparams) < 0) {
        fprintf(stderr, "Error snd_pcm_hw_params(handle, params)\n");
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);
    if (sndpcm->chunk_size == sndpcm->buffer_size) {
        fprintf(stderr, "Can't use period equal to buffer size (%lu == %lu)\n", sndpcm->chunk_size, sndpcm->buffer_size);
        return -1;
    }

    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format);
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * channels;
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;

    printf("\n---- WMix info -----\n   通道数: %d\n   采样率: %d Hz\n   采样位数: %d bit\n   总数据量: -- Bytes\n"
        "  每次写入帧数: %ld\n   每帧字节数: %ld Bytes\n   每次读写字节数: %ld Bytes\n   缓冲区大小: %ld Bytes\n\n", 
        channels, freq, sample,
        sndpcm->chunk_size,
        sndpcm->bits_per_frame/8,
        sndpcm->chunk_bytes,
        sndpcm->buffer_size);

    /* Allocate audio data buffer */
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
    if (!sndpcm->data_buf){
        fprintf(stderr, "Error malloc: [data_buf]\n");
        return -1;
    }

    return 0;
}

typedef struct{
    WMix_Struct *wmix;
    uint8_t flag;
    uint8_t *param;
}WMixThread_Param;

void wmix_throwOut_thread(
    WMix_Struct *wmix,
    uint8_t flag,
    uint8_t *param,
    size_t paramLen,
    void *callback)
{
    WMixThread_Param *wmtp;
    pthread_t th;
    pthread_attr_t attr;
    //
    wmtp = (WMixThread_Param*)calloc(1, sizeof(WMixThread_Param));
    wmtp->wmix = wmix;
    wmtp->flag = flag;
    if(paramLen > 0 && param)
    {
        wmtp->param = (uint8_t*)calloc(paramLen, sizeof(uint8_t));
        memcpy(wmtp->param, param, paramLen);
    }else
        wmtp->param = NULL;
    //attr init
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);   //禁用线程同步, 线程运行结束后自动释放
    //抛出线程
    pthread_create(&th, &attr, callback, (void*)wmtp);
    //attr destroy
    pthread_attr_destroy(&attr);
}

void wmix_load_stream_thread(WMixThread_Param *wmtp)
{
    char *path = (char*)&wmtp->param[4];
    //
    uint8_t chn = wmtp->param[0];
    uint8_t sample = wmtp->param[1];
    uint16_t freq = (wmtp->param[2]<<8) | wmtp->param[3];
    //
    int fd_read;
    uint8_t *buff;
    uint32_t buffSize;
    //
    WMix_Point src, head;
    ssize_t ret, total = 0;
    double total2 = 0, buffSizePow;
    uint32_t tick, second = 0, bytes_p_second, bytes_p_second2, bpsCount = 0;
    uint8_t rdce = wmtp->flag+1, rdceIsMe = 0;
    //
    if (mkfifo(path, 0666) < 0 && errno != EEXIST)
    {
        printf("wmix_stream_thread: mkfifo err\n");
        return;
    }
    //
    fd_read = open(path, O_RDONLY);
    //独占 reduceMode
    if(wmtp->flag && wmtp->wmix->reduceMode == 1)
    {
        wmtp->wmix->reduceMode = rdce;
        rdceIsMe = 1;
    }
    else
        rdce = 1;
    //
    bytes_p_second = chn*sample/8*freq;
    buffSize = bytes_p_second;
    buff = (uint8_t*)calloc(buffSize, sizeof(uint8_t));
    //
    bytes_p_second2 = WMIX_CHANNELS*WMIX_SAMPLE/8*WMIX_FREQ;
    buffSizePow = (double)bytes_p_second2/bytes_p_second;
    //
    printf("<< %s start >>\n   通道数: %d\n   采样位数: %d bit\n   采样率: %d Hz\n   每秒字节: %d Bytes\n\n", 
        path, chn, sample, freq, bytes_p_second);
    //
    src.U8 = buff;
    tick = wmtp->wmix->tick;
    head.U8 = wmtp->wmix->head.U8;
    //线程计数
    wmtp->wmix->thread_count += 1;
    //
    while(wmtp->wmix->run)
    {
        ret = read(fd_read, buff, buffSize);
        if(ret > 0)
        {
            //等播放指针赶上写入进度
            if(total2 > wmtp->wmix->playback->chunk_bytes)
            {
                while(wmtp->wmix->run && 
                    get_tick_err(wmtp->wmix->tick, tick) < 
                    total2 - wmtp->wmix->playback->chunk_bytes)
                    usleep(10000);
            }
            //
            head = wmix_load_wavStream(
                wmtp->wmix, 
                src, ret, freq, chn, sample, head, rdce);
            if(head.U8 == 0)
                break;
            //
            bpsCount += ret;
            total += ret;
            total2 = total*buffSizePow;
            //播放时间
            if(bpsCount > bytes_p_second)
            {
                bpsCount -= bytes_p_second;
                second = total/bytes_p_second;
                printf("  %s %02d:%02d\n", path, second/60, second%60);
            }
            continue;
        }
        else if(errno != EAGAIN)
            break;
        //
        usleep(10000);
    }
    //等待播放完毕
    // while(wmtp->wmix->run && 
    //     wmtp->wmix->head.U8 != wmtp->wmix->tail.U8 &&
    //     get_tick_err(wmtp->wmix->tick, tick) < total2)
    //     usleep(1000);
    //
    printf(">> %s end <<\n", path);
    //
    close(fd_read);
    //删除文件
    remove(path);
    //
    free(buff);
    //线程计数
    wmtp->wmix->thread_count -= 1;
    //
    if(wmtp->param)
        free(wmtp->param);
    //关闭 reduceMode
    if(rdceIsMe)
        wmtp->wmix->reduceMode = 1;
    free(wmtp);
}

void wmix_load_wav_mp3_thread(WMixThread_Param *wmtp)
{
    char *name = (char*)wmtp->param;
    uint16_t len = strlen((char*)wmtp->param);
    //线程计数
    wmtp->wmix->thread_count += 1;
    if(len > 3 &&
        (name[len-3] == 'm' || name[len-3] == 'M') &&
        (name[len-2] == 'p' || name[len-2] == 'P') &&
        name[len-1] == '3')
        wmix_load_mp3(wmtp->wmix, name, NULL, wmtp->flag);
    else
        wmix_load_wav(wmtp->wmix, name, NULL, wmtp->flag);
    //线程计数
    wmtp->wmix->thread_count -= 1;
    //
    if(wmtp->param)
        free(wmtp->param);
    free(wmtp);
}

void wmix_load_wav_mp3_2_thread(WMixThread_Param *wmtp)
{
    char *name = (char*)wmtp->param;
    uint16_t len = strlen((char*)wmtp->param);
    //线程计数
    wmtp->wmix->thread_count += 1;
    if(len > 3 &&
        (name[len-3] == 'm' || name[len-3] == 'M') &&
        (name[len-2] == 'p' || name[len-2] == 'P') &&
        name[len-1] == '3')
        wmix_load_mp3(wmtp->wmix, name, (char*)&wmtp->param[len+1], wmtp->flag);
    else
        wmix_load_wav(wmtp->wmix, name, (char*)&wmtp->param[len+1], wmtp->flag);
    //线程计数
    wmtp->wmix->thread_count -= 1;
    //
    if(wmtp->param)
        free(wmtp->param);
    free(wmtp);
}

void wmix_msg_thread(WMixThread_Param *wmtp)
{
    WMix_Struct *wmix = wmtp->wmix;
    WMix_Msg msg;
    ssize_t ret;

    //路径检查 //F_OK 是否存在 R_OK 是否有读权限 W_OK 是否有写权限 X_OK 是否有执行权限
    if(access(WMIX_MSG_PATH, F_OK) != 0)
        mkdir(WMIX_MSG_PATH, 0777);
    //再次检查
    if(access(WMIX_MSG_PATH, F_OK) != 0){
        fprintf(stderr, "wmix_msg_thread: msg path not found\n");
        return;
    }
    //清空文件夹
    system(WMIX_MSG_PATH_CLEAR);
    //权限处理
    system(WMIX_MSG_PATH_AUTHORITY);
    //获得管道
    if((wmix->msg_key = ftok(WMIX_MSG_PATH, WMIX_MSG_ID)) == -1){
        fprintf(stderr, "wmix_msg_thread: ftok err\n");
        return;
    }
    //清空队列
    if((wmix->msg_fd = msgget(wmix->msg_key, 0666)) != -1)
        msgctl(wmix->msg_fd, IPC_RMID, NULL);
    //重新创建队列
    if((wmix->msg_fd = msgget(wmix->msg_key, IPC_CREAT|0666)) == -1){
        fprintf(stderr, "wmix_msg_thread: msgget err\n");
        return;
    }
    //线程计数
    wmix->thread_count += 1;
    //接收来信
    while(wmix->run)
    {
        memset(&msg, 0, sizeof(WMix_Msg));
        ret = msgrcv(wmix->msg_fd, &msg, WMIX_MSG_BUFF_SIZE, 0, IPC_NOWAIT);//返回队列中的第一个消息 非阻塞方式
        if(ret > 0)
        {
            switch(msg.type&0xFF)
            {
                //音量设置
                case 1:
                    sys_volume_set(msg.value[0], msg.value[1]);
                    break;
                //混音播放音频
                case 2:
                    wmix_throwOut_thread(wmix, (msg.type>>8)&0xFF, msg.value, WMIX_MSG_BUFF_SIZE, &wmix_load_wav_mp3_thread);
                    break;
                //互斥播放音频
                case 3:
                    wmix_throwOut_thread(wmix, (msg.type>>8)&0xFF, msg.value, WMIX_MSG_BUFF_SIZE, &wmix_load_wav_mp3_2_thread);
                    break;
                //播放stream
                case 4:
                    wmix_throwOut_thread(wmix, (msg.type>>8)&0xFF, msg.value, WMIX_MSG_BUFF_SIZE, &wmix_load_stream_thread);
                    break;
                //复位
                case 5:
                    wmix->run = 0;
                    break;
            }
            continue;
        }
        //在别的地方重开了该程序的副本
        else if(ret < 1 && errno != ENOMSG)
        {
            fprintf(stderr, "wmix_msg_thread: process exist\n");
            exit(0);
        }
        //
        usleep(10000);
    }
    //删除队列
    msgctl(wmix->msg_fd, IPC_RMID, NULL);
    //
    printf("wmix_msg_thread exit\n");
    //线程计数
    wmix->thread_count -= 1;
    //
    if(wmtp->param)
        free(wmtp->param);
    free(wmtp);
}

void wmix_play_thread(WMixThread_Param *wmtp)
{
    WMix_Struct *wmix = wmtp->wmix;
    WMix_Point tail, dist;
    uint32_t count, divVal = WMIX_SAMPLE*WMIX_CHANNELS/8;
    SNDPCMContainer_t *playback = wmtp->wmix->playback;
    //线程计数
    wmix->thread_count += 1;
    //
    while(wmix->run)
    {
        if(wmix->head.U8 != wmix->tail.U8)
        {
            tail.U8 = wmix->tail.U8;
            //
            // printf("head: %ld, tail: %ld\n", 
            //     wmix->head.U8 - wmix->start.U8,
            //     tail.U8 - wmix->start.U8);
            //
            for(count = 0, dist.U8 = playback->data_buf; 
                wmix->head.U8 != tail.U8;)
            {
                if(wmix->head.U32 >= wmix->end.U32)
                    wmix->head.U32 = wmix->start.U32;
                //每次拷贝 4字节
                *dist.U32++ = *wmix->head.U32;
                *wmix->head.U32++ = 0;
                count += 4;
                //
                if(count == playback->chunk_bytes)
                {
                    wmix->tick += count;
                    //写入数据
                    SNDWAV_WritePcm(playback, count/divVal);
                    count = 0;
                    //
                    dist.U8 = playback->data_buf;
                    tail.U8 = wmix->tail.U8;
                }
            }
            //最后一丁点
            if(count)
            {
                wmix->tick += count;
                SNDWAV_WritePcm(playback, count/divVal);
            }
        }
        //
        usleep(1000);
    }
    //
    printf("wmix_play_thread exit\n");
    //线程计数
    wmix->thread_count -= 1;
    //
    if(wmtp->param)
        free(wmtp->param);
    free(wmtp);
}

WMix_Struct *wmix_init(void)
{
    char devicename[] = "default";

    WMix_Struct *wmix = NULL;
    SNDPCMContainer_t *playback = (SNDPCMContainer_t *)calloc(1, sizeof(SNDPCMContainer_t));

	//Creates a new output object using an existing stdio \c FILE pointer.
	if (snd_output_stdio_attach(&playback->log, stderr, 0) < 0)
	{
		fprintf(stderr, "Error snd_output_stdio_attach\n");
		goto Err;
	}
	// 打开PCM，最后一个参数为0意味着标准配置 SND_PCM_ASYNC
	if (snd_pcm_open(&playback->handle, devicename, SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		fprintf(stderr, "Error snd_pcm_open [ %s]\n", devicename);
		goto Err;
	}
	//配置PCM参数
	if (SNDWAV_SetParams2(playback, 
        WMIX_FREQ, WMIX_CHANNELS, WMIX_SAMPLE) < 0)
	{
		fprintf(stderr, "Error set_snd_pcm_params\n");
		goto Err;
	}
	snd_pcm_dump(playback->handle, playback->log);

	// SNDWAV_Play(&playback, &wav, fd);

    //
	wmix = (WMix_Struct *)calloc(1, sizeof(WMix_Struct));
    wmix->buff = (uint8_t *)calloc(WMIX_BUFF_SIZE+4, sizeof(uint8_t));
    wmix->playback = playback;
    wmix->head.U8 = wmix->tail.U8 = wmix->buff;
    wmix->start.U8 = wmix->buff;
    wmix->end.U8 = wmix->buff + WMIX_BUFF_SIZE;
    //
    // pthread_mutex_init(&wmix->lock, NULL);
    wmix->run = 1;
    wmix->reduceMode = 1;
    wmix_throwOut_thread(wmix, 0, NULL, 0, &wmix_msg_thread);
    wmix_throwOut_thread(wmix, 0, NULL, 0, &wmix_play_thread);
    //
	return wmix;

Err:

    if (playback->data_buf)
        free(playback->data_buf);
    if (playback->log)
        snd_output_close(playback->log);
    if (playback->handle)
        snd_pcm_close(playback->handle);
    //
    free(playback);

    return NULL;
}

void wmix_exit(WMix_Struct *wmix)
{
    int timeout;
    if(wmix)
    {
        wmix->run = 0;
        //等待线程关闭
        //等待各指针不再有人使用
        timeout = 200;//2秒超时
        do{
            if(timeout-- < 1)
                break;
            usleep(10000);
        }while(wmix->thread_count > 0);
        //
        if(wmix->playback)
        {
	        snd_pcm_drain(wmix->playback->handle);
            if(wmix->playback->data_buf)
                free(wmix->playback->data_buf);
            if(wmix->playback->log)
                snd_output_close(wmix->playback->log);
            if(wmix->playback->handle)
                snd_pcm_close(wmix->playback->handle);
            //
            free(wmix->playback);
        }
        //
        // pthread_mutex_destroy(&wmix->lock);
        free(wmix);
    }
}

static inline int16_t volumeAdd(int16_t L1, int16_t L2)
{
    int32_t sum;
    //
    if(L1 == 0)
        return L2;
    else if(L2 == 0)
        return L1;
    else{
        sum = (int32_t)L1 + L2;
        if(sum < -32768)
            return -32768;
        else if(sum > 32767)
            return 32767;
        else
            return sum;
    }
}

WMix_Point wmix_load_wavStream(
    WMix_Struct *wmix,
    WMix_Point src,
    uint32_t srcU8Len,
    uint16_t freq,
    uint8_t channels,
    uint8_t sample,
    WMix_Point head,
    uint8_t reduce)
{
    WMix_Point pHead = head, pSrc = src;
    //srcU8Len 计数
    uint32_t count;
    uint8_t *rdce = &wmix->reduceMode, rdce1 = 1;
    //
    if(!wmix || !wmix->run || !pSrc.U8 || !pHead.U8 || srcU8Len < 1)
    {
        pHead.U8 = 0;
        return pHead;
    }
    //
    if(reduce == wmix->reduceMode)
        rdce = &rdce1;
    //---------- 参数一致 直接拷贝 ----------
    if(freq == WMIX_FREQ && 
        channels == WMIX_CHANNELS && 
        sample == WMIX_SAMPLE)
    {
        for(count = 0; count < srcU8Len;)
        {
            //拷贝一帧数据
            *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
            pHead.S16++;
            pSrc.S16++;
            *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
            pHead.S16++;
            pSrc.S16++;
            //
            count += 4;
            //循环处理
            if(pHead.S16 >= wmix->end.S16)
                pHead.S16 = wmix->start.S16;
        }
    }
    //---------- 参数不一致 插值拷贝 ----------
    else
    {
        //频率差
        int32_t freqErr = freq - WMIX_FREQ;
        //步差计数 和 步差分量
        float divCount, divPow;
        //音频频率大于默认频率 //--- 重复代码比较多且使用可能极小,为减小函数入栈容量,不写了 ---
        if(freqErr > 0)
        {
            divPow = (float)freqErr/freq;
            //
            switch(sample)
            {
                case 8:
                    if(channels == 2)
                        ;
                    else if(channels == 1)
                        ;
                    break;
                case 16:
                    if(channels == 2)
                    {
                        for(count = 0, divCount = 0; count < srcU8Len;)
                        {
                            //步差计数已满 跳过帧
                            if(divCount >= 1.0)
                            {
                                //
                                pSrc.S16++;
                                pSrc.S16++;
                                //
                                divCount -= 1.0;
                                count += 4;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 4;
                            }
                            //循环处理
                            if(pHead.S16 >= wmix->end.S16)
                                pHead.S16 = wmix->start.S16;
                        }
                    }
                    else if(channels == 1)
                    {
                        for(count = 0, divCount = 0; count < srcU8Len;)
                        {
                            //步差计数已满 跳过帧
                            if(divCount >= 1.0)
                            {
                                //
                                pSrc.S16++;
                                //
                                divCount -= 1.0;
                                count += 2;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                // pSrc.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 2;
                            }
                            //循环处理
                            if(pHead.S16 >= wmix->end.S16)
                                pHead.S16 = wmix->start.S16;
                        }
                    }
                    break;
                case 32:
                    if(channels == 2)
                        ;
                    else if(channels == 1)
                        ;
                    break;
            }
        }
        //音频频率小于等于默认频率
        else
        {
            divPow = (float)(-freqErr)/freq;
            //
            // printf("smallFreq: head = %ld , divPow = %f, divCount = %f, freqErr/%d, freq/%d\n", 
            //     pHead.U8 - wmix->start.U8,
            //     divPow, divCount, freqErr, freq);
            //
            switch(sample)
            {
                //8bit采样 //--- 重复代码比较多且使用可能极小,为减小函数入栈容量,不写了 ---
                case 8:
                    if(channels == 2)
                        ;
                    else if(channels == 1)
                        ;
                    break;
                //16bit采样 //主流的采样方式
                case 16:
                    if(channels == 2)
                    {
                        for(count = 0, divCount = 0; count < srcU8Len;)
                        {
                            //步差计数已满 跳过帧
                            if(divCount >= 1.0)
                            {
                                //循环缓冲区指针继续移动,pSrc指针不动
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                //
                                divCount -= 1.0;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 4;
                            }
                            //循环处理
                            if(pHead.S16 >= wmix->end.S16)
                                pHead.S16 = wmix->start.S16;
                        }
                    }
                    else if(channels == 1)
                    {
                        for(count = 0, divCount = 0; count < srcU8Len;)
                        {
                            //
                            if(divCount >= 1.0)
                            {
                                //拷贝一帧数据 pSrc指针不动
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                //
                                divCount -= 1.0;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                // pSrc.S16++;
                                *pHead.S16 = volumeAdd(*pHead.S16, *pSrc.S16/(*rdce));
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 2;
                            }
                            //循环处理
                            if(pHead.S16 >= wmix->end.S16)
                                pHead.S16 = wmix->start.S16;
                        }
                    }
                    break;
                //32bit采样 //--- 重复代码比较多且使用可能极小,为减小函数入栈容量,不写了 ---
                case 32:
                    if(channels == 2)
                        ;
                    else if(channels == 1)
                        ;
                    break;
            }
        }
    }
    //---------- 修改尾指针 ----------
    if(pHead.U8 != head.U8)
    {
        if(wmix->tail.U8 < wmix->head.U8)
        {
            if(pHead.U8 < wmix->head.U8 && 
                pHead.U8 > wmix->tail.U8)
                wmix->tail.U8 = pHead.U8;
        }
        else
        {
            if(pHead.U8 < wmix->head.U8)
                wmix->tail.U8 = pHead.U8;
            else if(pHead.U8 > wmix->tail.U8)
                wmix->tail.U8 = pHead.U8;
        }
    }
    else
        pHead.U8 = 0;
    //
    return pHead;
}

void wmix_load_wav(
    WMix_Struct *wmix,
    char *wavPath,
    char *msgPath,
    uint8_t reduce)
{
    int fd = 0;
    ssize_t ret = 0;
    uint8_t *buff = NULL;
    uint32_t buffSize, buffSize2, buffSizeWait;
    WAVContainer_t wav;//wav文件头信息
    WMix_Point src, head;
    uint32_t tick, total = 0, total2 = 0, second = 0, bpsCount = 0;
    double totalPow;
    uint8_t rdce = reduce+1, rdceIsMe = 0;
    //
    WMix_Msg msg;
    key_t msg_key;
    int msg_fd;
    //
    if(!wmix || !wmix->run || !wavPath)
        return;
    //
    if((fd = open(wavPath, O_RDONLY)) <= 0)
    {
        fprintf(stderr, "wmix_load_wav: %s open err\n", wavPath);
        return;
    }
	if (WAV_ReadHeader(fd, &wav) < 0)
	{
		fprintf(stderr, "Error WAV_Parse [%s]\n", wavPath);
		close(fd);
        return;
	}
    //
    printf("<< %s start >>\n   通道数: %d\n   采样位数: %d bit\n   采样率: %d Hz\n   每秒字节: %d Bytes\n   总数据量: %d Bytes\n   msgPath: %s\n", 
        wavPath,
        wav.format.channels,
        wav.format.sample_length,
        wav.format.sample_rate,
        wav.format.bytes_p_second,
        wav.chunk.length,
        msgPath);
    //
    if(msgPath)
    {
        //创建消息挂靠路径
        if(access(msgPath, F_OK) != 0)
            creat(msgPath, 0777);
        //创建消息
        if((msg_key = ftok(msgPath, WMIX_MSG_ID)) == -1){
            fprintf(stderr, "wmix_load_wav2: ftok err\n");
            return;
        }
        if((msg_fd = msgget(msg_key, IPC_CREAT|0666)) == -1){
            fprintf(stderr, "wmix_load_wav2: msgget err\n");
            return;
        }
    }
    //独占 reduceMode
    if(reduce && wmix->reduceMode == 1)
    {
        wmix->reduceMode = rdce;
        rdceIsMe = 1;
    }
    else
        rdce = 1;
    //默认缓冲区大小设为1秒字播放字节数
    buffSize = wav.format.bytes_p_second;
    buffSize2 = WMIX_CHANNELS*WMIX_SAMPLE/8*WMIX_FREQ;
    totalPow = (double)buffSize2/buffSize;
    //把每秒数据包拆得越细, 打断速度越快
    //以下拆包的倍数必须能同时被 wav.format.sample_rate 和 WMIX_FREQ 整除 !!
    if(msgPath)//在互斥播放模式时才使用
    {
        if(wav.format.sample_rate%4 == 0)
        {
            buffSize /= 4;
            buffSize2 /= 4;
        }else if(wav.format.sample_rate%3 == 0)
        {
            buffSize /= 3;
            buffSize2 /= 3;
        }else if(wav.format.sample_rate%2 == 0)
        {
            buffSize /= 2;
            buffSize2 /= 2;
        }
    }
    buffSizeWait = buffSize2/2;
    //
    buff = (uint8_t *)calloc(buffSize, sizeof(uint8_t));
    //
    // SNDWAV_Play(wmix->playback, &wav, fd);
    //
    src.U8 = buff;
    head.U8 = wmix->head.U8;
    tick = wmix->tick;
    //
    do
    {
        //msg 检查
        if(msgPath){
            if(msgrcv(msg_fd, &msg, WMIX_MSG_BUFF_SIZE, 0, IPC_NOWAIT) < 1
                && errno != ENOMSG) //消息队列被关闭
                break;
        }
        //播放文件
        ret = read(fd, buff, buffSize);
        if(ret > 0)
        {
            //等播放指针赶上写入进度
            if(total2 > buffSizeWait)
            {
                while(wmix->run && 
                    get_tick_err(wmix->tick, tick) < 
                    total2 - buffSizeWait)
                    usleep(10000);
            }
            //写入循环缓冲区
            head = wmix_load_wavStream(
                wmix, 
                src, ret, 
                wav.format.sample_rate, 
                wav.format.channels, 
                wav.format.sample_length, head, rdce);
            //写入的总字节数统计
            bpsCount += ret;
            total += ret;
            total2 = total*totalPow;
            //播放时间
            if(bpsCount > wav.format.bytes_p_second)
            {
                bpsCount -= wav.format.bytes_p_second;
                second = total/wav.format.bytes_p_second;
                printf("  %s %02d:%02d\n", wavPath, second/60, second%60);
            }
            //
            if(head.U8 == 0)
                break;
        }
    }while(ret > 0);
    //等待播放完毕
    // while(wmix->run && 
    //     wmix->head.U8 != wmix->tail.U8 &&
    //     get_tick_err(wmix->tick, tick) < total2)
    //     usleep(1000);
    //
    if(msgPath)
        remove(msgPath);
    close(fd);
    free(buff);
    //
    printf(">> %s end <<\n", wavPath);
    //关闭 reduceMode
    if(rdceIsMe)
        wmix->reduceMode = 1;
}

typedef struct{
    char *msgPath;
    char *mp3Path;
    //
    WMix_Msg msg;
    // key_t msg_key;
    int msg_fd;
    //
    void *fdm;
    uint32_t seek;
    uint32_t size;
    //
    WMix_Point head, src;
    WMix_Struct *wmix;
    //
    uint32_t tick;
    uint32_t total, total2, totalWait;
    double totalPow;
    //
    uint32_t bps, bpsCount;
    //
    uint8_t rdce;
}WMix_Mp3;

inline int16_t mad_scale(mad_fixed_t sample)
{
    sample += (1L << (MAD_F_FRACBITS - 16));
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

enum mad_flow mad_output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
    WMix_Mp3 *wmm = data;
    uint32_t count = 0, second;
    int16_t *val = (int16_t*)&pcm->samples[0][count];
    //
    if(!wmm->wmix->run)
        return MAD_FLOW_STOP;
    //参数初始化
    if(wmm->head.U8 == 0)
    {
        wmm->bps = pcm->channels*16/8*header->samplerate;
        //
        wmm->totalPow = (double)(WMIX_CHANNELS*WMIX_SAMPLE/8*WMIX_FREQ)/wmm->bps;
        wmm->totalWait = wmm->bps*wmm->totalPow;
        //
        printf("<< %s start >>\n   通道数: %d\n   采样位数: %d bit\n   采样率: %d Hz\n   每秒字节: %d Bytes\n   总数据量: -- Bytes\n   msgPath: %s\n", 
            wmm->mp3Path,
            pcm->channels, 16,
            header->samplerate,
            wmm->bps,
            wmm->msgPath);
        //
        wmm->head.U8 = wmm->wmix->head.U8;
        wmm->tick = wmm->wmix->tick;
    }
    //msg 检查
    if(wmm->msgPath){
        if(msgrcv(wmm->msg_fd, &wmm->msg, WMIX_MSG_BUFF_SIZE, 0, IPC_NOWAIT) < 1
            && errno != ENOMSG) //消息队列被关闭
            return MAD_FLOW_STOP;
    }
    //
    if(pcm->channels == 2)
    {
        for(count = 0; count < pcm->length; count++)
        {
            *val++ = mad_scale(pcm->samples[0][count]);
            *val++ = mad_scale(pcm->samples[1][count]);
        }
        count = pcm->length*4;
    }
    else if(pcm->channels == 1)
    {
        for(count = 0; count < pcm->length; count++)
        {
            *val++ = mad_scale(pcm->samples[0][count]);
            // *val++ = mad_scale(pcm->samples[1][count]);
        }
        count = pcm->length*2;
    }
    else
        return MAD_FLOW_STOP;
    //等待消化
    if(wmm->total2 > wmm->totalWait)
    {
        while(wmm->wmix->run && 
            get_tick_err(wmm->wmix->tick, wmm->tick) < 
            wmm->total2 - wmm->totalWait)
            usleep(10000);
    }
    //写入到循环缓冲区
    wmm->src.U8 = (uint8_t*)&pcm->samples[0][0];
    wmm->head = wmix_load_wavStream(
        wmm->wmix, 
        wmm->src, 
        count,
        header->samplerate,
        pcm->channels,
        16,
        wmm->head, wmm->rdce);
    //总字节数计数
    wmm->bpsCount += count;
    wmm->total += count;
    wmm->total2 = wmm->total*wmm->totalPow;
    //播放时间
    if(wmm->bpsCount > wmm->bps)
    {
        wmm->bpsCount -= wmm->bps;
        second = wmm->total/wmm->bps;
        printf("  %s %02d:%02d\n", wmm->mp3Path, second/60, second%60);
    }
    //
    if(wmm->head.U8 == 0)
        return MAD_FLOW_STOP;
    //
    return MAD_FLOW_CONTINUE;
}

enum mad_flow mad_input(void *data, struct mad_stream *stream)
{
    WMix_Mp3 *wmm = data;
    if(wmm->size > 0)
    {
        mad_stream_buffer(stream, wmm->fdm+wmm->seek, wmm->size);
        wmm->size = 0;
        return MAD_FLOW_CONTINUE;
    }
    return MAD_FLOW_STOP;
}

enum mad_flow mad_error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
    fprintf(stderr, "decoding error 0x%04x (%s)\n",
        stream->error,
        mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}

void wmix_load_mp3(
    WMix_Struct *wmix,
    char *mp3Path,
    char *msgPath,
    uint8_t reduce)
{
    // WMix_Msg msg;
    key_t msg_key;
    // int msg_fd;
    uint8_t rdceIsMe = 0;
    //
    struct stat sta;
    int fd;
    //
    WMix_Mp3 wmm;
    struct mad_decoder decoder;
    
    //参数准备
    memset(&wmm, 0, sizeof(WMix_Mp3));
    wmm.wmix = wmix;
    wmm.mp3Path = mp3Path;
    wmm.rdce = reduce+1;
    
    //
    if(msgPath)
    {
        //创建消息挂靠路径
        if(access(msgPath, F_OK) != 0)
            creat(msgPath, 0777);
        //创建消息
        if((msg_key = ftok(msgPath, WMIX_MSG_ID)) == -1){
            fprintf(stderr, "wmix_load_mp3: ftok err\n");
            return;
        }
        if((wmm.msg_fd = msgget(msg_key, IPC_CREAT|0666)) == -1){
            fprintf(stderr, "wmix_load_mp3: msgget err\n");
            return;
        }
        //
        wmm.msgPath = msgPath;
    }

    //
    if((fd = open(mp3Path, O_RDONLY)) <= 0){
        fprintf(stderr, "wmix_load_mp3: open %s err\n", mp3Path);
        return;
    }
    if(fstat(fd, &sta) == -1 || sta.st_size == 0){
        fprintf(stderr, "wmix_load_mp3: stat %s err\n", mp3Path);
        close(fd);
        return;
    }
    
    //跳过id3标签
    wmm.seek = id3_len(mp3Path);
    wmm.size = sta.st_size - wmm.seek;
    // lseek(fd, wmm.seek, SEEK_SET);

    //
    wmm.fdm = mmap(0, sta.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(wmm.fdm == MAP_FAILED || !wmm.fdm){
        fprintf(stderr, "wmix_load_mp3: mmap err\n");
        close(fd);
        return;
    }
    
    //独占 reduceMode
    if(reduce && wmix->reduceMode == 1)
    {
        wmix->reduceMode = wmm.rdce;
        rdceIsMe = 1;
    }
    else
        wmm.rdce = 1;

    //configure input, output, and error functions
    mad_decoder_init(&decoder, &wmm,
        mad_input, 0 /* header */, 0 /* filter */, mad_output,
        mad_error, 0 /* message */);
    
    //start decoding
    mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

    //release the decoder
    mad_decoder_finish(&decoder);

    //关闭 reduceMode
    if(rdceIsMe)
        wmix->reduceMode = 1;

    //等待播放完毕
    // while(wmm.wmix->run && 
    //     wmix->head.U8 != wmix->tail.U8 &&
    //     get_tick_err(wmm.wmix->tick, wmm.tick) < wmm.total2)
    //     usleep(1000);
    //
    if(msgPath)
        remove(msgPath);
    close(fd);
    munmap(wmm.fdm, sta.st_size);
    //
    printf(">> %s end <<\n", mp3Path);
}

//--------------- wmix main ---------------

int main(int argc, char **argv)
{
    // record_wav("./capture.wav", 5);
    WMix_Struct *wmix = wmix_init();
    if(wmix){
        sleep(1);
        while(1){
            //--- 重启 ---
            if(wmix->run == 0 && wmix->thread_count == 0){
                wmix->run = 1;
                wmix_throwOut_thread(wmix, 0, NULL, 0, &wmix_msg_thread);
                wmix_throwOut_thread(wmix, 0, NULL, 0, &wmix_play_thread);
            }
            usleep(100000);
        }
        wmix_exit(wmix);
    }
    return 0;
}
