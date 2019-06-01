#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "wav.h"

/*******************************************************************************
 * 名称: WAV_P_FmtString
 * 功能: fmt数值转为字符串
 * 参数: fmt： 需转换的数值     
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
static char *WAV_P_FmtString(uint16_t fmt)
{
    switch (fmt) {
    case WAV_FMT_PCM:
        return "PCM";
        break;
    case WAV_FMT_IEEE_FLOAT:
        return "IEEE FLOAT";
        break;
    case WAV_FMT_DOLBY_AC3_SPDIF:
        return "DOLBY AC3 SPDIF";
        break;
    case WAV_FMT_EXTENSIBLE:
        return "EXTENSIBLE";
        break;
    default:
        break;
    }

    return "NON Support Fmt";
}
/*******************************************************************************
 * 名称: WAV_P_PrintHeader
 * 功能: 打印wav文件相关信息
 * 参数: container： WAVContainer_t结构体指针     
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
static void WAV_P_PrintHeader(WAVContainer_t *container)
{
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("File Magic:         [%c%c%c%c]\n",
        (char)(container->header.magic),
        (char)(container->header.magic>>8),
        (char)(container->header.magic>>16),
        (char)(container->header.magic>>24));
    printf("File Length:        [%d]\n", container->header.length);
    printf("File Type:          [%c%c%c%c]\n",
        (char)(container->header.type),
        (char)(container->header.type>>8),
        (char)(container->header.type>>16),
        (char)(container->header.type>>24));
    printf("Fmt Magic:          [%c%c%c%c]\n",
        (char)(container->format.magic),
        (char)(container->format.magic>>8),
        (char)(container->format.magic>>16),
        (char)(container->format.magic>>24));
    printf("Fmt Size:           [%d]\n", container->format.fmt_size);
    printf("Fmt Format:         [%s]\n", WAV_P_FmtString(container->format.format));
    printf("Fmt Channels:       [%d]\n", container->format.channels);
    printf("Fmt Sample_rate:    [%d](HZ)\n", container->format.sample_rate);
    printf("Fmt Bytes_p_second: [%d]\n", container->format.bytes_p_second);
    printf("Fmt Blocks_align:   [%d]\n", container->format.blocks_align);
    printf("Fmt Sample_length:  [%d]\n", container->format.sample_length);
    printf("Chunk Type:         [%c%c%c%c]\n",
        (char)(container->chunk.type),
        (char)(container->chunk.type>>8),
        (char)(container->chunk.type>>16),
        (char)(container->chunk.type>>24));
    printf("Chunk Length:       [%d]\n", container->chunk.length);
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}

/*******************************************************************************
 * 名称: WAV_P_CheckValid
 * 功能: wav文件格式检测
 * 参数: container： WAVContainer_t结构体指针     
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
static int WAV_P_CheckValid(WAVContainer_t *container)
{
    if (container->header.magic != WAV_RIFF ||
        container->header.type != WAV_WAVE ||
        container->format.magic != WAV_FMT ||
        container->format.fmt_size != LE_INT(16) ||
        (container->format.channels != LE_SHORT(1) && container->format.channels != LE_SHORT(2)) ||
        container->chunk.type != WAV_DATA) {

        fprintf(stderr, "non standard wav file.\n");
        WAV_P_PrintHeader(container);
        return -1;
    }

    return 0;
}
/*******************************************************************************
 * 名称: WAV_ReadHeader
 * 功能: wav头文件读取
 * 参数: container： WAVContainer_t结构体指针     
 *            fd： 文件句柄
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int WAV_ReadHeader(int fd, WAVContainer_t *container)
{
    assert((fd >=0) && container);

    if (read(fd, &container->header, sizeof(container->header)) != sizeof(container->header) ||
        read(fd, &container->format, sizeof(container->format)) != sizeof(container->format) ||
        read(fd, &container->chunk, sizeof(container->chunk)) != sizeof(container->chunk)) {

        fprintf(stderr, "Error WAV_ReadHeader\n");
        return -1;
    }

    if (WAV_P_CheckValid(container) < 0)
        return -1;

#ifdef WAV_PRINT_MSG
    WAV_P_PrintHeader(container);
#endif

    return 0;
}
/*******************************************************************************
 * 名称: WAV_WriteHeader
 * 功能: wav头文件写入
 * 参数: container： WAVContainer_t结构体指针     
 *            fd： 文件句柄
 * 返回: 0：正常 -1:错误
 * 说明: 无
 ******************************************************************************/
int WAV_WriteHeader(int fd, WAVContainer_t *container)
{
    assert((fd >=0) && container);

    if (WAV_P_CheckValid(container) < 0)
        return -1;

    if (write(fd, &container->header, sizeof(container->header)) != sizeof(container->header) ||
        write(fd, &container->format, sizeof(container->format)) != sizeof(container->format) ||
        write(fd, &container->chunk, sizeof(container->chunk)) != sizeof(container->chunk)) {

        fprintf(stderr, "Error WAV_WriteHeader\n");
        return -1;
    }

#ifdef WAV_PRINT_MSG
    WAV_P_PrintHeader(container);
#endif

    return 0;
}
