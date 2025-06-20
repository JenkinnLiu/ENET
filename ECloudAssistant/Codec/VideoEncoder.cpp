/**
 * @file VideoEncoder.cpp
 * @brief H.264 视频编码器实现
 *
 * 实现了 VideoEncoder 类的方法，包括编码器初始化、像素转换
 * 以及帧数据发送与接收流程，输出 H.264 码流包。
 */

#include "VideoEncoder.h"
#include "VideoConvert.h"

extern "C"
{
    #include <libavutil/rational.h>
    #include <libavutil/error.h>
    #include<libavutil/opt.h>
}

/**
 * @brief 构造函数
 *
 * 分配用于转换前的 RGBA AVFrame 和输出 H.264 AVPacket。
 */
VideoEncoder::VideoEncoder()
    :pts_(0)
    ,width_(0)
    ,height_(0)
    ,force_idr_(false)
    ,rgba_frame_(nullptr)
    ,h264_packet_(nullptr)
    ,converter_(nullptr)
{
    // 创建并管理原始像素帧
    rgba_frame_.reset(av_frame_alloc(),[](AVFrame* ptr){av_frame_free(&ptr);});
    // 创建并管理编码输出包
    h264_packet_.reset(av_packet_alloc(),[](AVPacket* ptr){av_packet_free(&ptr);});
}

/**
 * @brief 析构函数
 *
 * 调用 Close 释放编码器及转换器资源。
 */
VideoEncoder::~VideoEncoder()
{
    Close();
}

/**
 * @brief 初始化 H.264 编码器
 * @param video_config 编码参数（宽度、高度、帧率、码率等）
 * @return 成功返回 true，失败返回 false
 *
 * 步骤：
 *  1. 查找并创建 libavcodec 的 H.264 编码器上下文
 *  2. 设置宽高、像素格式(YUV420P)、帧率、GOP 大小等
 *  3. 配置低延迟选项（zerolatency、ultrafast）
 *  4. 调用 avcodec_open2 打开编码器
 */
bool VideoEncoder::Open(AVConfig &video_config)
{
    //初始化
    if(is_initialzed_)
    {
        Close();
    }

    config_ = video_config;

    //查找编码器H264
    codec_ = const_cast<AVCodec*>(avcodec_find_encoder(AV_CODEC_ID_H264));
    if(!codec_)
    {
        Close();
        return false;
    }

    //创建编码器上下文
    codecContext_ = avcodec_alloc_context3(codec_);
    if(!codecContext_)
    {
        Close();
        return false;
    }

    //配置上下文参数
    codecContext_->width = config_.video.width;
    codecContext_->height = config_.video.height;
    codecContext_->time_base = {1,(qint32)config_.video.framerate};//帧率倒数
    codecContext_->framerate = {(qint32)config_.video.framerate,1};
    codecContext_->gop_size = 30;
    codecContext_->max_b_frames = 0;//降低延迟
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->bit_rate = config_.video.bitrate;
    //还需要设置全局头
    codecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //还需要加上参数，降低编码延迟
    codecContext_->rc_min_rate = config_.video.bitrate;
    codecContext_->rc_max_rate = config_.video.bitrate;
    codecContext_->rc_buffer_size = config_.video.bitrate;
    //设置字典
    av_opt_set(codecContext_->priv_data,"tune","zerolatency",0);//极速编码
    av_opt_set(codecContext_->priv_data,"preset","ultrafast",0);//极速编码

    //打开编码器
    if(avcodec_open2(codecContext_,codec_,NULL) != 0)
    {
        Close();
        return false;
    }

    width_ = config_.video.width;
    height_ = config_.video.height;
    is_initialzed_ = true;
    return true;
}

/**
 * @brief 关闭编码器并清理资源
 */
void VideoEncoder::Close()
{
    // 重置宽高和PTS状态
    width_ = 0;
    height_ = 0;
    pts_ = 0;
    is_initialzed_ = false;
    if(converter_)
    {
        converter_->Close();
        converter_.reset();
        converter_ = nullptr;
    }
}

/**
 * @brief 对一帧像素数据进行编码
 * @param data 原始像素缓冲（如 RGBA）
 * @param width 缓冲宽度
 * @param height 缓冲高度
 * @param data_size 数据大小（字节）
 * @param pts 帧时间戳，可选，默认为自动递增
 * @return 成功返回 AVPacketPtr，内含 H.264 码流；失败返回 nullptr
 *
 * 流程：
 *  1. 若分辨率或格式与目标不符，创建并初始化 VideoConverter
 *  2. 将原始 data 数据拷贝到 rgba_frame_
 *  3. 调用 converter_->Convert 得到 YUV420P AVFrame
 *  4. 使用 avcodec_send_frame / avcodec_receive_packet 进行编码
 */
AVPacketPtr VideoEncoder::Encode(const quint8 *data, quint32 width, quint32 height, quint32 data_size, quint64 pts)
{
    //开始编码
    if(!is_initialzed_)
    {
        return nullptr;
    }

    //此外我们需要去初始化转换器，如果这个输入宽高跟目标不一致我们就需要创建转换器
    if(width_ != width || height_ != height || !converter_)
    {
        converter_.reset(new VideoConverter());
        //初始化视频转换器
        if(!converter_->Open(width,height,(AVPixelFormat)config_.video.format,
                              codecContext_->width,codecContext_->height,codecContext_->pix_fmt))
        {
            //初始化失败
            converter_.reset();
            converter_ = nullptr;
            return nullptr;
        }

        rgba_frame_->width = width_;
        rgba_frame_->height = height_;
        //指定格式
        rgba_frame_->format = config_.video.format;
        //获取内存
        if(av_frame_get_buffer(rgba_frame_.get(),32) != 0)//四字节rgba
        {
            return nullptr;
        }
    }

    //我们将输入数据转到这个rgbaframe来去转换
    memcpy(rgba_frame_->data[0],data,data_size);

    //转换
    if(!converter_)
    {
        return nullptr;
    }

    //开始转换
    //准备输出
    AVFramePtr out_frame = nullptr;
    if(converter_->Convert(rgba_frame_,out_frame) <= 0)
    {
        return nullptr;
    }

    //更新out_frame参数
    out_frame->pts = pts >= 0 ? pts : pts_++;
    out_frame->pict_type = AV_PICTURE_TYPE_NONE;

    //再去编码
    if(avcodec_send_frame(codecContext_,out_frame.get()) < 0)
    {
        return nullptr;
    }
    //我们再去接收这个值
    int ret = avcodec_receive_packet(codecContext_,h264_packet_.get());
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        return nullptr;
    }
    else if(ret < 0)
    {
        return nullptr;
    }
    return h264_packet_;
}
