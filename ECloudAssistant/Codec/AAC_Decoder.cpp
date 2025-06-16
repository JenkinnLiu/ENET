#include "AAC_Decoder.h"
#include "Audio_Resampler.h"

AAC_Decoder::AAC_Decoder(AVContext *ac, QObject *parent)
    :QThread(parent)
    ,avContext_(ac)
{
    audioResampler_.reset(new AudioResampler());
}

AAC_Decoder::~AAC_Decoder()
{
    Close();
}

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
