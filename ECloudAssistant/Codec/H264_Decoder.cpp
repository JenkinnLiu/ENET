#include "H264_Decoder.h"
#include "VideoConvert.h"

H264_Decoder::H264_Decoder(AVContext *ac, QObject *parent)
    :QThread(parent)
    ,avContext_(ac)
{
    videoConver_.reset(new VideoConverter());
    yuv_frame_ = AVFramePtr(av_frame_alloc(),[](AVFrame* p){av_frame_free(&p);});
}

H264_Decoder::~H264_Decoder()
{
    Close();
}

int H264_Decoder::Open(const AVCodecParameters *codecParamer)
{
    //i
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

    //创建解码器上下文
    codecCtx_ = avcodec_alloc_context3(codec_);
    //复制解码器参数到上下文
    if(avcodec_parameters_to_context(codecCtx_,codecParamer) < 0)
    {
        return -1;
    }

    //设置加速属性
    codecCtx_->flags |= AV_CODEC_FLAG2_FAST;

    //打开解码器
    if(avcodec_open2(codecCtx_,codec_,nullptr) != 0)
    {
        return -1;
    }

    //更新这个上下文
    avContext_->video_width = codecCtx_->width;
    avContext_->video_height = codecCtx_->height;
    avContext_->video_fmt = AV_PIX_FMT_YUV420P;

    //初始化视频转换器
    if(!videoConver_->Open(codecCtx_->width,codecCtx_->height,codecCtx_->pix_fmt,
                            codecCtx_->width,codecCtx_->height,AV_PIX_FMT_YUV420P))
    {
        return -1;
    }
    is_initial_ = true;
    //开始解码
    start();
    return 0;
}

void H264_Decoder::Close()
{
    quit_ = true;
    if(isRunning())
    {
        this->quit();
        this->wait();
    }
}

void H264_Decoder::run()
{
    //解码线程
    AVPacketPtr pkt = nullptr;
    while(!quit_ && videoConver_)
    {
        if(!video_queue_.size())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        //这个队列有数据，就开始去pop
        video_queue_.pop(pkt);
        if(avcodec_send_packet(codecCtx_,pkt.get()))
        {
            //如果是不为0，erro
            break;
        }
        int ret = 0;
        while(ret >= 0)
        {
            //开始接收frame
            ret = avcodec_receive_frame(codecCtx_,yuv_frame_.get());
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            //转换视频帧
            AVFramePtr outFrame = nullptr;
            if(videoConver_->Convert(yuv_frame_,outFrame))
            {
                if(outFrame)
                {
                    //添加这个帧到帧队列
                    avContext_->video_queue_.push(outFrame);
                }
            }
        }
    }
}














