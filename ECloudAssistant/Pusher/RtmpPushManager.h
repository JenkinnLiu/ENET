/**
 * @file RtmpPushManager.h
 * @brief 管理音视频采集、编码与 RTMP 推流的调度器
 *
 * RtmpPushManager 负责屏幕和麦克风音频的采集、
 * 使用 H.264/AAC 编码后，通过 RTMP 推流器发送到服务器。
 */

#ifndef RTMPPUSHMANAGER_H
#define RTMPPUSHMANAGER_H
#include <thread>
#include <memory>
#include "RtmpPublisher.h"
#include "H264Encoder.h"
#include <QObject>

class AACEncoder;
class AudioCapture;
class GDIScreenCapture;
/**
 * @class RtmpPushManager
 * @brief 音视频 RTMP 推流管理类
 *
 * 集成了屏幕采集、音频采集、视频/音频编码和 RTMP 发送，
 * 支持多线程并行处理并管理推流全流程。
 */
class RtmpPushManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 析构函数，关闭推流并释放所有资源
     */
    virtual ~RtmpPushManager();

    /**
     * @brief 构造函数，创建事件循环实例
     */
    RtmpPushManager();

    /**
     * @brief 打开并启动推流
     * @param str RTMP 推流地址（如 rtmp://host/app/stream）
     * @return 成功返回 true，失败返回 false
     */
    bool Open(const QString& str);

    /**
     * @brief 判断是否已关闭推流
     * @return true 表示已关闭，false 表示正在推流中
     */
    bool isClose(){return isConnect == false;}

protected:
    /**
     * @brief 初始化采集器、编码器和推流器
     * @return 成功返回 true，失败返回 false
     */
    bool Init();

    /**
     * @brief 停止推流并清理所有资源
     */
    void Close();

    /**
     * @brief 视频采集并编码后通过 RTMP 推送循环
     */
    void EncodeVideo();

    /**
     * @brief 音频采集并编码后通过 RTMP 推送循环
     */
    void EncodeAudio();

    /**
     * @brief 停止编码线程并释放编码器资源
     */
    void StopEncoder();

    /**
     * @brief 停止采集线程并释放采集器资源
     */
    void StopCapture();

    /**
     * @brief 判断一段 H.264 数据是否关键帧
     * @param data NAL 单元数据指针
     * @param size 数据长度（字节）
     * @return true 表示关键帧，false 表示非关键帧
     */
    bool IsKeyFrame(const uint8_t* data, uint32_t size);

    /**
     * @brief 发送编码后的视频数据（去除起始码）
     * @param data H.264 NALU 数据指针
     * @param size 数据长度（字节）
     */
    void PushVideo(const quint8* data, quint32 size);

    /**
     * @brief 发送编码后的音频数据
     * @param data AAC 原始帧数据指针
     * @param size 数据长度（字节）
     */
    void PushAudio(const quint8* data, quint32 size);

private:
    bool exit_ = false;                                 ///< 推流退出标志
    bool isConnect = false;                             ///< 推流连接状态
    EventLoop* loop_ = nullptr;                        ///< 事件循环实例
    std::unique_ptr<AACEncoder>  aac_encoder_;          ///< AAC 编码器
    std::unique_ptr<H264Encoder> h264_encoder_;         ///< H.264 编码器
    std::shared_ptr<RtmpPublisher> pusher_;             ///< RTMP 推流器实例
    std::unique_ptr<AudioCapture> audio_Capture_;       ///< 音频采集器
    std::unique_ptr<GDIScreenCapture> screen_Capture_;  ///< 屏幕采集器
    std::unique_ptr<std::thread>  audioCaptureThread_;  ///< 音频采集线程
    std::unique_ptr<std::thread>  videoCaptureThread_;  ///< 视频采集线程
};

#endif // RTMPPUSHMANAGER_H
