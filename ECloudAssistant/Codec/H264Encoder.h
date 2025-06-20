/**
 * @file H264Encoder.h
 * @brief 定义 H264Encoder 类，封装 H.264 视频编码流程，提供打开、关闭、编码及获取编码参数接口
 * @date 2025-06-19
 */
#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include <QtGlobal>
#include <vector>
#include "AV_Common.h"

class VideoEncoder;

/**
 * @class H264Encoder
 * @brief H.264 视频编码封装，基于 VideoEncoder 实现，将 RGBA 原始图像编码为 H.264 码流
 */
class H264Encoder
{
public:
    /**
     * @brief 构造函数，初始化内部 VideoEncoder 实例
     */
    H264Encoder();

    H264Encoder(const H264Encoder&) = delete;           ///< 禁用拷贝构造
    H264Encoder& operator=(const H264Encoder&) = delete;///< 禁用拷贝赋值

    /**
     * @brief 析构函数，调用 Close 清理资源
     */
    ~H264Encoder();

public:
    /**
     * @brief 打开并初始化 H.264 编码器
     * @param width 输入图像宽度（像素）
     * @param height 输入图像高度（像素）
     * @param framerate 帧率（fps）
     * @param bitrate 码率（kbps）
     * @param format 像素格式（AVPixelFormat 枚举值）
     * @return true 初始化成功，false 初始化失败
     */
    bool OPen(qint32 width, qint32 height, qint32 framerate, qint32 bitrate, qint32 format);

    /**
     * @brief 关闭编码器并释放资源
     */
    void Close();

    /**
     * @brief 对 RGBA 数据进行 H.264 编码
     * @param rgba_buffer 指向原始 RGBA 像素数据的指针
     * @param width 原始图像宽度
     * @param height 原始图像高度
     * @param size 原始数据字节大小
     * @param out_frame 输出编码后的 H.264 码流数据容器
     * @return 返回编码后数据的字节长度，失败返回 -1
     */
    qint32 Encode(quint8* rgba_buffer, quint32 width, quint32 height, quint32 size, std::vector<quint8>& out_frame);

    /**
     * @brief 获取编码器的序列参数集（SPS/PPS）
     * @param out_buffer 输出缓冲区，用于接收序列参数
     * @param out_buffer_size 缓冲区大小（字节）
     * @return 返回实际拷贝的字节数，失败返回 -1
     */
    qint32 GetSequenceParams(quint8* out_buffer, qint32 out_buffer_size);

private:
    /**
     * @brief 判断编码包是否为关键帧
     * @param pkt 编码后的 AVPacketPtr
     * @return true 是关键帧，false 否
     */
    bool IsKeyFrame(AVPacketPtr pkt);

private:
    AVConfig config_;                                 ///< 编码器参数配置
    std::unique_ptr<VideoEncoder> h264_encoder_;      ///< 底层 VideoEncoder 实例
};

#endif // H264_ENCODER_H
