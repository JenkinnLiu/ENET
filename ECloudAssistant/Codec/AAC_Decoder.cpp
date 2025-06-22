/**
 * @file AAC_Decoder.cpp
 * @brief AAC 音频解码器实现
 *
 * 实现 AAC_Decoder 类，包括解码器打开、异步解码循环、
 * 重采样以及将 PCM 帧推送到 AVContext 队列的逻辑。
 */

#include "AAC_Decoder.h"
#include "Audio_Resampler.h"

/**
 * @brief 构造函数
 * @param ac 外部 AVContext 指针，用于存储解码结果
 * @param parent 可选的 QObject 父对象指针
 */
AAC_Decoder::AAC_Decoder(AVContext *ac, QObject *parent)
    :QThread(parent)
    ,avContext_(ac)
{
    audioResampler_.reset(new AudioResampler());
}

/**
 * @brief 析构函数
 *
 * 调用 Close() 停止解码线程并释放资源。
 */
AAC_Decoder::~AAC_Decoder()
{
    Close();
}

/**
 * @brief 打开 AAC 解码器并初始化重采样器
 * @param codecParamer AVCodecParameters 指针，包含编码参数
 * @return 0 成功，-1 失败
 *
 * 步骤：
 *  1. 查找并创建 AAC 解码器上下文
 *  2. 将参数复制到解码器上下文
 *  3. 打开解码器并启用快速解码标志
 *  4. 配置目标采样率、声道和样本格式
 *  5. 打开重采样器并启动解码线程
 */
int AAC_Decoder::Open(const AVCodecParameters *codecParamer)
{
    //初始化解码器，通过codecParamer解码参数来去初始化
    if(is_initial_ || !codecParamer)
    {
        return -1;
    }

    //创建解码器
    codec_ = const_cast<AVCodec*>(avcodec_find_decoder(codecParamer->codec_id));
    if(!codec_)
    {
        return -1;
    }

    //我们去创建一个解码器上下文
    codecCtx_ = avcodec_alloc_context3(codec_);
    //复制这个解码器参数 codecParamer->codecCtx_
    if(avcodec_parameters_to_context(codecCtx_,codecParamer) < 0)//赋值失败
    {
        return -1;
    }
    //设置一个属性，来去加速这个解码速度
    codecCtx_->flags |= AV_CODEC_FLAG2_FAST;
    //打开解码器
    if(avcodec_open2(codecCtx_,codec_,nullptr) != 0)
    {
        return -1;
    }

    //初始化这个重采样
    avContext_->audio_channels_layout = AV_CH_LAYOUT_STEREO;//立体声
    avContext_->audio_fmt = AV_SAMPLE_FMT_S16;
    avContext_->audio_sample_rate = 44100;
    //打开这个重采样
    if(!audioResampler_->Open(codecCtx_->sample_rate,codecCtx_->channels,codecCtx_->sample_fmt,
                             44100,2,AV_SAMPLE_FMT_S16))
    {
        return -1;
    }
    is_initial_ = true;
    //启动线程开始解码
    start();
    return 0;
}

/**
 * @brief 关闭解码器线程并释放资源
 */
void AAC_Decoder::Close()
{
    //将标志位置为true
    quit_ = true;
    if(isRunning())
    {
        this->quit();
        this->wait();
    }
}

/**
 * @brief 解码线程入口函数
 *
 * 从队列中取出 AAC 数据包，调用 avcodec_send_packet
 * 和 avcodec_receive_frame 解码并对每帧做重采样，
 * 最后将 PCM 数据帧推送到 AVContext 的音频队列。
 */
void AAC_Decoder::run()
{
    //解码线程
    int ret = -1;
    //准备这个avpacket
    AVPacketPtr pkt = nullptr;
    //输出帧
    AVFramePtr outframe = nullptr; //重采样之后的音频帧
    AVFramePtr pFrame = AVFramePtr(av_frame_alloc(),[](AVFrame* p){av_frame_free(&p);});//解码帧
    while(!quit_ && audioResampler_)
    {
        //获取这个包队列
        if(!audio_queue_.size()) //为空，我们就要去休眠
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        //pop
        audio_queue_.pop(pkt);
        //开始解码aac
        if(avcodec_send_packet(codecCtx_,pkt.get()))
        {
            //如果不为0，error
            break;
        }
        //开始接收
        while(true)
        {
            ret = avcodec_receive_frame(codecCtx_,pFrame.get());
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if(ret < 0)
            {
                return;
            }
            else
            {
                //处理帧
                //须要重采样
                if(audioResampler_->Convert(pFrame,outframe))
                {
                    //重采样成功
                    if(outframe)
                    {
                        avContext_->audio_queue_.push(outframe);
                    }
                }
                //再将帧数据填充到
            }
        }
    }

}
