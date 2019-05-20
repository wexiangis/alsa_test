
wmix
功能：托管alsa的混音小程序
编译依赖：
    wav.c/h
    wmix.c/h
    id3.c/h
    mad.h 
    alsa音频库(ubuntu下使用 sudo apt-get install libalsa* 安装)
    mad mp3解码库(ubuntu下使用 sudo apt-get install libmad* 安装)
使用：抛到后台运行，用户程序通过使用 wmix_user.c/h 里提供的接口方法即可播放音频


test
功能：测试程序
编译依赖：
    main.c
    wmix_user.c/h
使用：根据 main 函数内容进行播放音频操作


----- wmix_user.c/h 接口功能详细说明: -----


void wmix_set_volume(uint8_t count, uint8_t div);
功能：音量设置
参数：
    count：音量分数的分子
    div：音量分数的分分母
例子：
    wmix_set_volume(30, 100);//设置 30% 的音量
    

void wmix_play(char *wavOrMp3);
功能：播放 wav 或者 mp3 音频文件
参数：
    wavOrMp3：wav 或者 mp3 音频文件文件路径
其他说明：
    重复调用该函数播放音频，可实现音频叠加。
    另外特别提醒，有些音频本身具有高音高频段，叠加后很快就会破音(由于喇叭无法发出这些声音)，
    这个问题可以通过叠加前先降低音量来稍微缓解，本程序的权衡做法是 (L1/3 + L2/3)*2


void wmix_play2(char *wavOrMp3);
功能：互斥的播放 wav 或者 mp3 音频文件
参数：
    wavOrMp3：wav 或者 mp3 音频文件文件路径
其他说明：
    1.所谓互斥，即当你播放下一首时，上一首如果没有结束将直接被关闭
    2.wmix_play2(NULL);可以关闭当前播放


int wmix_stream_open(uint8_t channels, uint8_t sample, uint16_t freq);
功能：得到fifo的写fd，写入音频流
参数：
    channels：通道数，常见值为1和2，即单声道和双声道(立体声)
    sample：采样位宽bit，常见值8、16、32
    freq：采样频率Hz，常见值44100、32000、22050、16000、11025、8000、6000
返回：
    成功返回fifo写fd
    失败返回0
例子：
    //得到fifo写fd，参数按你实际音频情况定
    if((fd = wmix_stream_open(2, 16, 44100)) > 0)
    {
        //写入音频流
        write(fd, buff, buffSize);
        ...
        //关闭fifo
        close(fd);
    }


void wmix_reset(void);
功能：
    1.复位 wmix，届时将结束所有的播放，重启所有对外的消息队列和fifo
    2.注意，如果复位过程中有进程仍在使用fifo写数据，该进程会因为写阻塞而被关闭
    

