/**
 * @file H264Encoder.cpp
 * @brief 实现 H264Encoder 类，封装视频编码流程，将 RGBA 图像编码为 H.264 码流
 * @date 2025-06-19
 */
#include "H264Encoder.h"
#include "VideoEncoder.h"

/**
 * @brief 构造函数，创建底层 VideoEncoder 实例并初始化配置
 */
H264Encoder::H264Encoder()
    :config_{}
    ,h264_encoder_(nullptr)
{
    h264_encoder_.reset(new VideoEncoder());
}

/**
 * @brief 析构函数，关闭编码器并释放资源
 */
H264Encoder::~H264Encoder()
{
    Close();
}

/**
 * @brief 打开并初始化 H.264 编码器
 * @param width 视频宽度（像素）
 * @param height 视频高度（像素）
 * @param framerate 帧率（fps）
 * @param bitrate 码率（kbps）
 * @param format 像素格式（AVPixelFormat 枚举值）
 * @return true 表示打开并初始化成功，false 表示失败
 */
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

/**
 * @brief 关闭编码器并释放底层资源
 */
void H264Encoder::Close()
{
    h264_encoder_->Close();
}

/**
 * @brief 对输入 RGBA 数据进行 H.264 编码
 * @param rgba_buffer 指向 RGBA 原始像素数据的指针
 * @param width 图像宽度（像素）
 * @param height 图像高度（像素）
 * @param size 原始数据字节大小
 * @param out_frame 输出编码后的 H.264 码流容器
 * @return 返回编码后数据的字节长度，失败返回 -1
 */
qint32 H264Encoder::Encode(quint8 *rgba_buffer, quint32 width, quint32 height, quint32 size, std::vector<quint8> &out_frame)
{
    //编码264
    out_frame.clear();
    int frame_size = 0;
    // 设置最大输出大小
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

/**
 * @brief 获取编码器的序列参数集（SPS/PPS），用于解码器初始化
 * @param out_buffer 输出缓冲区指针
 * @param out_buffer_size 缓冲区大小（字节）
 * @return 实际写入的字节数，失败返回 -1
 */
qint32 H264Encoder::GetSequenceParams(quint8 *out_buffer, qint32 out_buffer_size)
{
    //获取编码参数
    quint32 size = 0;
    if(!h264_encoder_->GetAVCodecContext())
    {
        return -1;
    }
    // 检查输出缓冲区大小
    AVCodecContext* codecContxt = h264_encoder_->GetAVCodecContext();
    size = codecContxt->extradata_size;
    // 复制编码参数到输出缓冲区
    memcpy(out_buffer,codecContxt->extradata,codecContxt->extradata_size);
    return size;
}

/**
 * @brief 判断编码后的数据包是否为关键帧
 * @param pkt 编码后的 AVPacketPtr
 * @return true 为关键帧，false 否
 */
bool H264Encoder::IsKeyFrame(AVPacketPtr pkt)
{
    //判断是否为关键帧
    return pkt->flags & AV_PKT_FLAG_KEY;
}
