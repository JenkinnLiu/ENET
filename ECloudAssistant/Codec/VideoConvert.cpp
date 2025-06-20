/**
 * @file VideoConvert.cpp
 * @brief VideoConverter 类方法实现
 *
 * 实现了基于 FFmpeg libswscale 的视频帧像素格式与分辨率转换逻辑。
 */

#include "VideoConvert.h"

extern "C"
{
    #include<libavformat/avformat.h>
    #include<libswscale/swscale.h>
}

/**
 * @brief 构造函数，初始化成员变量为默认无效状态
 */
VideoConverter::VideoConverter()
    :width_(0)
    ,height_(0)
    ,format_(AV_PIX_FMT_NONE)
    ,swsContext_(nullptr)
{

}

/**
 * @brief 打开并初始化 FFmpeg 转换上下文
 * @param in_width 输入帧宽度（像素）
 * @param in_height 输入帧高度（像素）
 * @param in_format 输入帧像素格式
 * @param out_width 输出帧宽度（像素）
 * @param out_height 输出帧高度（像素）
 * @param out_format 输出帧像素格式
 * @return 初始化成功返回 true，否则返回 false
 */
bool VideoConverter::Open(qint32 in_width, qint32 in_height, AVPixelFormat in_format,
                          qint32 out_width, qint32 out_height, AVPixelFormat out_format)
{
    // 转换器已存在时不能再次打开
    if(swsContext_)
    {
        return false;
    }

    // 创建并配置 SwsContext，用于像素格式及分辨率转换
    swsContext_ = sws_getContext(in_width, in_height, in_format,
                                 out_width, out_height, out_format,
                                 SWS_BICUBIC, NULL, NULL, NULL);
    // 保存输出参数
    width_ = out_width;
    height_ = out_height;
    format_ = out_format;

    return swsContext_ != nullptr;
}

/**
 * @brief 关闭并释放转换上下文资源
 */
void VideoConverter::Close()
{
    if(swsContext_)
    {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }
}

/**
 * @brief 执行单帧像素格式和分辨率转换
 * @param in_frame 输入 AVFrame 智能指针，包含原始帧数据
 * @param out_frame 输出 AVFrame 智能指针引用，用于接收转换后帧
 * @return 返回转换输出的行数（>=0），若失败返回 -1
 */
qint32 VideoConverter::Convert(AVFramePtr in_frame, AVFramePtr &out_frame)
{
    // 确保转换上下文已正确初始化
    if(!swsContext_)
    {
        return -1;
    }

    // 分配并管理输出帧内存
    out_frame.reset(av_frame_alloc(), [](AVFrame* ptr){ av_frame_free(&ptr); });

    // 设置输出帧属性
    out_frame->width = width_;
    out_frame->height = height_;
    out_frame->format = format_;
    out_frame->pts = in_frame->pts;
    out_frame->pkt_dts = in_frame->pkt_dts;

    // 分配数据缓冲区
    if(av_frame_get_buffer(out_frame.get(), 0) != 0)
    {
        return -1;
    }

    // 执行格式和尺寸转换，返回实际处理的行数
    uint32_t height_slice = sws_scale(swsContext_, in_frame->data, in_frame->linesize,
                                      0, in_frame->height,
                                      out_frame->data, out_frame->linesize);
    if(height_slice < 0)
    {
        return -1;
    }
    return height_slice;
}

/**
 * @brief 析构函数，释放转换资源
 */
VideoConverter::~VideoConverter()
{
    Close();
}
