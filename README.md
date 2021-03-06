
----- 编译说明: -----


#编译依赖库(交叉编译自行启用Makefile内的cross)
make libs

#生成目标文件 wmix 和 test
make


----- 生成文件说明: -----


wmix
功能：托管alsa的混音小程序
编译依赖：
    wav.c/h
    wmix.c/h
    id3.c/h
    mad.h 
    alsa音频库
    mad mp3解码库
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
    

void wmix_play(char *wavOrMp3, uint8_t backgroundReduce);
功能：播放 wav 或者 mp3 音频文件
参数：
    wavOrMp3：wav 或者 mp3 音频文件文件路径
    backgroundReduce：播放当前音频时,降低背景音量
        0: 不启用
        >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
        注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得)
其他说明：
    重复调用该函数播放音频，可实现音频叠加。


void wmix_play2(char *wavOrMp3, uint8_t backgroundReduce, uint8_t repeatInterval);
功能：互斥的播放 wav 或者 mp3 音频文件
参数：
    wavOrMp3：wav 或者 mp3 音频文件文件路径
    backgroundReduce：播放当前音频时,降低背景音量
        0: 不启用
        >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
        注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得)
    repeatInterval：音频重复播放间隔,单位 sec
        0: 不启用
        >0: 播放结束后间隔 repeatInterval 秒后重播
其他说明：
    1.所谓互斥，即当你播放下一首时，上一首如果没有结束将直接被关闭
    2.wmix_play2(NULL, 0, 0);可以关闭当前播放


int wmix_stream_open(uint8_t channels, uint8_t sample, uint16_t freq, uint8_t backgroundReduce);
功能：得到fifo的写fd，写入音频流
参数：
    channels：通道数，常见值为1和2，即单声道和双声道(立体声)
    sample：采样位宽bit，常见值8、16、32
    freq：采样频率Hz，常见值44100、32000、22050、16000、11025、8000、6000
    backgroundReduce：播放当前音频时,降低背景音量
        0: 不启用
        >0: 背景音量降低倍数 backgroundVolume/(backgroundReduce+1)
        注意: 当有进程正在使用backgroundReduce功能时,当前启用无效(先占先得)
返回：
    成功返回fifo写fd
    失败返回0
例子：
    //得到fifo写fd，参数按你实际音频情况定
    if((fd = wmix_stream_open(2, 16, 44100, 0)) > 0)
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
    

