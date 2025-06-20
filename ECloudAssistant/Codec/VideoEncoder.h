/**
 * @file VideoEncoder.h
 * @brief H.264 视频编码器类定义
 *
 * 封装了从原始像素数据到 H.264 码流的完整编码流程，
 * 内部支持像素格式转换(VideoConverter)和 libavcodec 编码。
 */

#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H
#include <memory>
#include "AV_Common.h"
#include "VideoConvert.h"

/**
 * @class VideoEncoder
 * @brief H.264 视频帧编码器
 *
 * 继承自 EncodBase，实现对输入的像素数据执行像素格式转换
 * 及 H.264 编码，输出 AVPacketPtr。可配置分辨率、帧率、码率等参数。
 */
class VideoEncoder : public EncodBase
{
public:
    /**
     * @brief 构造函数，初始化内部变量并分配 AVFrame/AVPacket
     */
    VideoEncoder();

    /**
     * @brief 析构函数，调用 Close 释放资源
     */
    ~VideoEncoder();

    /** 禁用拷贝构造和赋值运算符 */
    VideoEncoder(const VideoEncoder&) = delete;
    VideoEncoder& operator=(const VideoEncoder&) = delete;

public:
    /**
     * @brief 打开并初始化编码器
     * @param video_config 编码器配置，包含分辨率、帧率、码率等
     * @return 成功返回 true，失败返回 false
     *
     * 步骤：
     *  1. 查找 H.264 编码器并分配上下文
     *  2. 设置宽高、像素格式、帧率、码率等参数
     *  3. 配置低延迟选项（zerolatency、ultrafast）
     *  4. 打开编码器
     */
    virtual bool Open(AVConfig& video_config) override;

    /**
     * @brief 关闭编码器并清理资源
     */
    virtual void Close() override;

    /**
     * @brief 将一帧原始像素数据编码为 H.264 码流包
     * @param data 原始像素缓冲区指针（如 RGBA）
     * @param width 输入帧宽度（像素）
     * @param height 输入帧高度（像素）
     * @param data_size 像素数据大小（字节）
     * @param pts 帧时间戳，若为默认值 0 则内部自动递增
     * @return 成功返回 AVPacketPtr 编码数据包，失败返回 nullptr
     *
     * 流程：
     *  1. 像素格式/尺寸不一致时初始化或重置 VideoConverter
     *  2. 拷贝原始数据到 rgba_frame_
     *  3. 调用 converter_->Convert 生成 YUV420P AVFrame
     *  4. avcodec_send_frame -> avcodec_receive_packet 获取输出包
     */
    virtual AVPacketPtr Encode(const quint8* data,quint32 width,quint32 height,quint32 data_size,quint64 pts = 0);

private:
    qint64  pts_;             ///< 当前帧的 PTS
    quint32 width_;           ///< 目标编码宽度
    quint32 height_;          ///< 目标编码高度
    bool    force_idr_;       ///< 是否强制关键帧（未使用）
    AVFramePtr         rgba_frame_;   ///< 存放原始像素的临时帧
    AVPacketPtr        h264_packet_;  ///< 存放输出 H.264 数据包
    std::unique_ptr<VideoConverter> converter_; ///< 像素格式/尺寸转换器
};
#endif // VIDEO_ENCODER_H
