#include "H264Encoder.h"
#include "VideoEncoder.h"

H264Encoder::H264Encoder()
    :config_{}
    ,h264_encoder_(nullptr)
{
    h264_encoder_.reset(new VideoEncoder());
}

H264Encoder::~H264Encoder()
{
    Close();
}

bool H264Encoder::OPen(qint32 width, qint32 height, qint32 framerate, qint32 bitrate, qint32 format)
{
    //初始化编码器
    config_.video.width = width;
    config_.video.height = height;
    config_.video.framerate = framerate;
    config_.video.bitrate = bitrate * 1000;
    config_.video.gop = framerate;
    config_.video.format = (AVPixelFormat)format;
    return h264_encoder_->Open(config_);
}

void H264Encoder::Close()
{
    h264_encoder_->Close();
}

qint32 H264Encoder::Encode(quint8 *rgba_buffer, quint32 width, quint32 height, quint32 size, std::vector<quint8> &out_frame)
{
    //编码264
    out_frame.clear();
    int frame_size = 0;
    int max_out_size = config_.video.width * config_.video.height * 4;//设大一点 因为这个编码数据不会大于这个原始数据RGBA
    //申请内存
    std::shared_ptr<quint8> out_buffer(new quint8[max_out_size],std::default_delete<quint8[]>());
    //开始编码
    AVPacketPtr pkt = h264_encoder_->Encode(rgba_buffer,width,height,size);
    if(!pkt)
    {
        //编码失败
        return -1;
    }
    quint32 extra_size = 0;
    quint8* extra_data = nullptr;
    //判断是否是关键帧 如果是关键帧需要在264前面添加编码信息
    if(IsKeyFrame(pkt))
    {
        //添加编码信息
        //先去获取编码信息
        extra_data = h264_encoder_->GetAVCodecContext()->extradata;
        extra_size = h264_encoder_->GetAVCodecContext()->extradata_size;
        //编码信息放到包头去解析
        memcpy(out_buffer.get(),extra_data,extra_size);
        frame_size += extra_size;
    }
    memcpy(out_buffer.get() + frame_size,pkt->data,pkt->size); //264[编码信息 + 264裸流]
    frame_size += pkt->size;

    //需要将数据传出去out_frame
    if(frame_size > 0)
    {
        out_frame.resize(frame_size);
        out_frame.assign(out_buffer.get(),out_buffer.get() + frame_size);
        return frame_size;
    }
    return 0;
}

qint32 H264Encoder::GetSequenceParams(quint8 *out_buffer, qint32 out_buffer_size)
{
    //获取编码参数
    quint32 size = 0;
    if(!h264_encoder_->GetAVCodecContext())
    {
        return -1;
    }
    AVCodecContext* codecContxt = h264_encoder_->GetAVCodecContext();
    size = codecContxt->extradata_size;
    memcpy(out_buffer,codecContxt->extradata,codecContxt->extradata_size);
    return size;
}

bool H264Encoder::IsKeyFrame(AVPacketPtr pkt)
{
    //判断是否为关键帧
    return pkt->flags & AV_PKT_FLAG_KEY;
}
