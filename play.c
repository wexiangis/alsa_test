
#include <unistd.h>

#include "play.h"
#include "wav.h"

/*******************************************************************************
 * 名称: sys_volume_set
 * 功能: 扬声器音量设置
 * 参数: vol_value 设置的音量值 (范围：0-10之间)
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int sys_volume_set(uint8_t vol_value)
{
    long volume_value;
    snd_mixer_t * mixer;
    snd_mixer_elem_t *pcm_element;

    volume_value = (long)vol_value;
    snd_mixer_open(&mixer, 0);
    snd_mixer_attach(mixer, "default");
    snd_mixer_selem_register(mixer, NULL, NULL);
    snd_mixer_load(mixer);
    //找到Pcm对应的element
    pcm_element = snd_mixer_first_elem(mixer);/* 取得第一个 element，也就是 Master */
    if(volume_value>10)
        volume_value = 10;
    if(volume_value<0)
        volume_value = 0;

    snd_mixer_selem_set_playback_volume_range(pcm_element, 0, 10);//设置音量范围：0-100之间
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_LEFT,volume_value) < 0)
        return -1;
    //右音量
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_RIGHT,volume_value) < 0)
       return -1;
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_LEFT,volume_value) < 0)//设置两次，防止设置不成功
        return -1;
    //右音量
    if(snd_mixer_selem_set_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_RIGHT,volume_value) < 0)
        return -1;
    
    printf("------------------set_playback_volume:%ld\r\n", volume_value);

    snd_mixer_selem_get_playback_volume(pcm_element,SND_MIXER_SCHN_FRONT_LEFT,&volume_value);//获取音量
    
    printf("------------------get_playback_volume:%ld\r\n", volume_value);

    //处理事件
    snd_mixer_handle_events(mixer);
    snd_mixer_close(mixer);
    return 0;

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
            fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
        } else if (r == -ESTRPIPE) {
            fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
        } else if (r < 0) {
            fprintf(stderr, "Error snd_pcm_writei: [%s]", snd_strerror(r));
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
            fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
        } else if (r == -ESTRPIPE) {
            fprintf(stderr, "<<<<<<<<<<<<<<< Need suspend >>>>>>>>>>>>>>>\n");
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

    // printf("---- wav info -----\n  通道数: %d\n  采样率: %d Hz\n  采样位数: %d bit\n  总数据量: %ld Bytes\n"
    //     "  每次写入帧数: %ld\n  每帧字节数: %ld Bytes\n  每次读写字节数: %ld Bytes\n  缓冲区大小: %d Bytes\n", 
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
    off64_t rest;
    size_t c, frame_size;

    if (WAV_WriteHeader(fd, wav) < 0) {
        return ;
    }

    rest = wav->chunk.length;
    while (rest > 0) {
        c = (rest <= (off64_t)sndpcm->chunk_bytes) ? (size_t)rest : sndpcm->chunk_bytes;
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
    int i = 0, bit_count = 4;
    int16_t *valueP;

    int load, ret;
    off64_t written = 0;
    off64_t c;
    off64_t count = LE_INT(wav->chunk.length);

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
        // ret = SNDWAV_WritePcm(sndpcm, load);
        // ret = SNDWAV_WritePcm(sndpcm, load);
        // ret = SNDWAV_WritePcm(sndpcm, load);
        // ret = SNDWAV_WritePcm(sndpcm, load);
        // ret = SNDWAV_WritePcm(sndpcm, load);
        // ret = SNDWAV_WritePcm(sndpcm, load);

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

//----------------------------- XXX -----------------------------

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
        format = SND_PCM_FORMAT_S16_LE;
    
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

    printf("---- wav info -----\n  通道数: %d\n  采样率: %d Hz\n  采样位数: %d bit\n  总数据量: -- Bytes\n"
        "  每次写入帧数: %ld\n  每帧字节数: %ld Bytes\n  每次读写字节数: %ld Bytes\n  缓冲区大小: %ld Bytes\n", 
        channels, freq, sample,
        sndpcm->chunk_size,
        sndpcm->bits_per_frame/8,
        sndpcm->chunk_bytes,
        sndpcm->buffer_size);

    /* Allocate audio data buffer */
    sndpcm->data_buf = (uint8_t *)malloc(sndpcm->chunk_bytes);
    if (!sndpcm->data_buf) {
        fprintf(stderr, "Error malloc: [data_buf]\n");
        return -1;
    }

    return 0;
}

void circle_msg_thread(SNDPCMContainer2_t *playback2)
{
    while(playback2->run)
    {
        usleep(10000);
    }
    //
    printf("circle_msg_thread exit\n");
}

void circle_play_thread(SNDPCMContainer2_t *playback2)
{
    CircleBuff_Point tail, dist;
    uint32_t count, divVal = DEFAULT_CIRCLE_SAMPLE*DEFAULT_CIRCLE_CHANNELS/8;
    SNDPCMContainer_t *playback = playback2->playback;
    //
    while(playback2->run)
    {
        if(playback2->head.U8 != playback2->tail.U8)
        {
            tail.U8 = playback2->tail.U8;
            //
            // printf("head: %ld, tail: %ld\n", 
            //     playback2->head.U8 - playback2->start.U8,
            //     tail.U8 - playback2->start.U8);
            //
            for(count = 0, dist.U8 = playback->data_buf; 
                playback2->head.U8 != tail.U8;)
            {
                if(playback2->head.U32 >= playback2->end.U32)
                    playback2->head.U32 = playback2->start.U32;
                //每次拷贝 4字节
                *dist.U32++ = *playback2->head.U32;
                *playback2->head.U32++ = 0;
                count += 4;
                //
                if(count == playback->chunk_bytes)
                {
                    playback2->tick += count;
                    //写入数据
                    SNDWAV_WritePcm(playback, count/divVal);
                    count = 0;
                    //
                    dist.U8 = playback->data_buf;
                    tail.U8 = playback2->tail.U8;
                }
            }
            //最后一丁点
            if(count)
            {
                playback2->tick += count;
                SNDWAV_WritePcm(playback, count/divVal);
            }
        }
        //
        usleep(1000);
    }
    //
    printf("circle_play_thread exit\n");
}

SNDPCMContainer2_t *circle_play_init(void)
{
    char devicename[] = "default";

    SNDPCMContainer2_t *playback2 = NULL;
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
        DEFAULT_CIRCLE_FREQ, DEFAULT_CIRCLE_CHANNELS, DEFAULT_CIRCLE_SAMPLE) < 0)
	{
		fprintf(stderr, "Error set_snd_pcm_params\n");
		goto Err;
	}
	snd_pcm_dump(playback->handle, playback->log);

	// SNDWAV_Play(&playback, &wav, fd);
	// snd_pcm_drain(playback->handle);

    //
	playback2 = (SNDPCMContainer2_t *)calloc(1, sizeof(SNDPCMContainer2_t));
    playback2->buff = (uint8_t *)calloc(DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE+4, sizeof(uint8_t));
    playback2->playback = playback;
    playback2->head.U8 = playback2->tail.U8 = playback2->buff;
    playback2->start.U8 = playback2->buff;
    playback2->end.U8 = playback2->buff + DEFAULT_CIRCLE_CIRCLE_BUFF_SIZE;
    //
    // pthread_mutex_init(&playback2->lock, NULL);
    playback2->run = 1;
    pthread_create(&playback2->th_msg, NULL, (void*)circle_msg_thread, (void*)playback2);
    pthread_create(&playback2->th_paly, NULL, (void*)circle_play_thread, (void*)playback2);
    //
	return playback2;

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

void circle_play_exit(SNDPCMContainer2_t *playback2)
{
    if(playback2)
    {
        playback2->run = 0;
        //等待线程关闭
        pthread_join(playback2->th_msg, NULL);
        pthread_join(playback2->th_paly, NULL);
        //等待各指针不再有人使用
        sleep(1);
        //
        if(playback2->playback)
        {
            if(playback2->playback->data_buf)
                free(playback2->playback->data_buf);
            if(playback2->playback->log)
                snd_output_close(playback2->playback->log);
            if(playback2->playback->handle)
                snd_pcm_close(playback2->playback->handle);
            //
            free(playback2->playback);
        }
        //
        // pthread_mutex_destroy(&playback2->lock);
        free(playback2);
    }
}

CircleBuff_Point circle_play_load_wavStream(
    SNDPCMContainer2_t *playback2,
    CircleBuff_Point src,
    uint32_t srcU8Len,
    uint32_t freq,
    uint8_t channels,
    uint8_t sample,
    CircleBuff_Point head)
{
    CircleBuff_Point pHead = head, pSrc = src;
    CircleBuff_Point pTail;
    //
    uint8_t reduceBackground = 3, reduceSrc = 3;
    uint8_t recover = 2;
    //
    if(!playback2 || !playback2->run || !pSrc.U8 || !pHead.U8 || srcU8Len < 1)
    {
        pHead.U8 = 0;
        return pHead;
    }
    //---------- 参数一致 直接拷贝 ----------
    if(freq == DEFAULT_CIRCLE_FREQ && 
        channels == DEFAULT_CIRCLE_CHANNELS && 
        sample == DEFAULT_CIRCLE_SAMPLE)
    {
        //--- 数据拷贝 ---
        if(pHead.U8 + srcU8Len < playback2->end.U8)//数据正常写入
        {
            pTail.U32 = pHead.U32 + srcU8Len/4 + 1;
            //
            for(;pHead.S16 < pTail.S16;)
            {
                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                *pHead.S16 *= recover;
                pHead.S16++;
                pSrc.S16++;
            }
        }
        else//数据将从循环缓冲区尾部折返到头部
        {
            pTail.U32 = playback2->start.U32 + (srcU8Len - (playback2->end.U8 - pHead.U8))/4 + 1;
            //写到尾
            for(;pHead.S16 < playback2->end.S16;)
            {
                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                *pHead.S16 *= recover;
                pHead.S16++;
                pSrc.S16++;
            }
            //从头部继续写
            for(pHead.S16 = playback2->start.S16; pHead.S16 < pTail.S16;)
            {
                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                *pHead.S16 *= recover;
                pHead.S16++;
                pSrc.S16++;
            }
        }
    }
    //---------- 参数不一致 插值拷贝 ----------
    else
    {
        //频率不一致带来的 步差
        int32_t freqErr = freq - DEFAULT_CIRCLE_FREQ;
        float divCount, divPow;
        uint32_t count;
        //音频频率大于默认频率
        if(freqErr > 0)
        {
            divPow = (float)freqErr/freq;
            //
            // printf("largeFreq: head = %ld , divPow = %f, divCount = %f, freqErr/%d, freq/%d\n", 
            //     pHead.U8 - playback2->start.U8,
            //     divPow, divCount, freqErr, freq);
            //
            //----- 懒得支持 减小函数入栈容量 -----
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
                        ;
                    else if(channels == 1)
                        ;
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
            //     pHead.U8 - playback2->start.U8,
            //     divPow, divCount, freqErr, freq);
            //
            //----- 这里只写了16bit采样的数据 减小函数入栈容量 -----
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
                            //
                            if(divCount >= 1.0)
                            {
                                //拷贝一帧数据 pSrc指针不动
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                //
                                divCount -= 1.0;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                pSrc.S16++;
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 4;
                            }
                            //循环处理
                            if(pHead.S16 >= playback2->end.S16)
                                pHead.S16 = playback2->start.S16;
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
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                //
                                divCount -= 1.0;
                            }
                            else
                            {
                                //拷贝一帧数据
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                // pSrc.S16++;
                                *pHead.S16 = (*pHead.S16)/reduceBackground + (*pSrc.S16)/reduceSrc;
                                *pHead.S16 *= recover;
                                pHead.S16++;
                                pSrc.S16++;
                                //
                                divCount += divPow;
                                count += 2;
                            }
                            //循环处理
                            if(pHead.S16 >= playback2->end.S16)
                                pHead.S16 = playback2->start.S16;
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
    }
    //---------- 修改尾指针 ----------
    if(pHead.U8 != head.U8)
    {
        if(playback2->tail.U8 < playback2->head.U8)
        {
            if(pHead.U8 < playback2->head.U8 && 
                pHead.U8 > playback2->tail.U8)
                playback2->tail.U8 = pHead.U8;
        }
        else
        {
            if(pHead.U8 < playback2->head.U8)
                playback2->tail.U8 = pHead.U8;
            else if(pHead.U8 > playback2->tail.U8)
                playback2->tail.U8 = pHead.U8;
        }
    }
    else
        pHead.U8 = 0;
    //
    return pHead;
}

void circle_play_load_wav(
    SNDPCMContainer2_t *playback2,
    char *wavPath)
{
    int fd = 0;
    int ret = 0;
    uint8_t *buff = NULL;
    uint32_t buffSize, buffSize2, buffSizeWait;
    WAVContainer_t wav;//wav文件头信息
    CircleBuff_Point src, head;
    uint32_t tick, sum = 0, sum2 = 0, second = 0;
    uint32_t chunk_bytes, chunk_num;
    //
    if(!playback2 || !playback2->run || !wavPath)
        return;
    //
    if((fd = open(wavPath, O_RDONLY)) <= 0)
    {
        fprintf(stderr, "circle_play_load_wav: %s open err\n", wavPath);
        return;
    }
    //
	if (WAV_ReadHeader(fd, &wav) < 0)
	{
		fprintf(stderr, "Error WAV_Parse [%s]\n", wavPath);
		close(fd);
        return;
	}
    //
    printf("%s :\n  通道数: %d\n  采样位数: %d bit\n  采样率: %d Hz\n  每秒字节: %d Bytes\n  总数据量: %d Bytes\n\n", 
        wavPath,
        wav.format.channels,
        wav.format.sample_length,
        wav.format.sample_rate,
        wav.format.bytes_p_second,
        wav.chunk.length);
    //
    chunk_bytes = playback2->playback->chunk_bytes
        *wav.format.channels/DEFAULT_CIRCLE_CHANNELS
        *wav.format.sample_length/DEFAULT_CIRCLE_SAMPLE
        *wav.format.sample_rate/DEFAULT_CIRCLE_FREQ;
    chunk_num = DEFAULT_WAV_CACHE_BUFF_SIZE/playback2->playback->chunk_bytes;
    //
    buffSize = chunk_bytes*chunk_num;
    buffSize2 = playback2->playback->chunk_bytes*chunk_num;
    buffSizeWait = playback2->playback->chunk_bytes*(chunk_num-3);
    //
    buff = (uint8_t *)calloc(buffSize, sizeof(uint8_t));
    //
    // SNDWAV_Play(playback2->playback, &wav, fd);
    //
    src.U8 = buff;
    head.U8 = playback2->head.U8;
    tick = playback2->tick;
    //
    do
    {
        ret = read(fd, buff, buffSize);
        if(ret > 0)
        {
            //等播放指针赶上写入进度
            if(sum2 > 0)
            {
                while(get_tick_err(playback2->tick, tick) < 
                    sum2 - buffSizeWait)
                    usleep(10000);
            }
            //写入的总字节数统计
            sum += ret;
            sum2 += buffSize2;
            //写入循环缓冲区
            head = circle_play_load_wavStream(
                playback2, 
                src, ret, 
                wav.format.sample_rate, 
                wav.format.channels, 
                wav.format.sample_length, head);
            //播放时间
            second = sum/wav.format.bytes_p_second;
            //
            printf("%s : tail: %ld -- head: %ld  %d bytes  %02d:%02d\n", 
                wavPath,
                playback2->tail.U8 - playback2->start.U8,
                playback2->head.U8 - playback2->start.U8,
                sum, second/60, second%60);
            //
            if(head.U8 == 0)
                break;
        }
    }while(ret > 0);
    //
    close(fd);
    if(buff)
        free(buff);
    //
    printf("%s : end\n", wavPath);
}

