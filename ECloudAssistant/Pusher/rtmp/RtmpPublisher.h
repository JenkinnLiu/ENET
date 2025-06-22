/**
 * @file RtmpPublisher.h
 * @brief RTMP 推流器类定义
 *
 * 继承自 Rtmp 基类，实现基于 RTMP 协议的音视频推流功能，
 * 支持设置媒体信息、打开推流地址、发送音视频帧、关闭连接等。
 */

#ifndef RTMPPUBLISHER_H
#define RTMPPUBLISHER_H
#include "rtmp.h"
#include "EventLoop.h"
#include "TimeStamp.h"
#include "RtmpConnection.h"

/**
 * @class RtmpPublisher
 * @brief RTMP 视频/音频推送器
 *
 * 封装 RTMP 推流流程，包括握手、媒体参数设置、
 * 首帧 SPS/PPS/AAC 配置发送、关键帧识别、时间戳管理等。
 */
class RtmpPublisher : public Rtmp, public std::enable_shared_from_this<RtmpPublisher>
{
public:
    /**
     * @brief 创建 RtmpPublisher 实例
     * @param loop 事件循环，用于任务调度
     * @return 返回 RtmpPublisher 共享指针
     */
    static std::shared_ptr<RtmpPublisher> Create(EventLoop* loop);

    /**
     * @brief 析构函数，自动关闭连接并释放资源
     */
    ~RtmpPublisher();

    /**
     * @brief 设置音视频媒体信息
     * @param media_info 媒体信息，包括编码类型、SPS/PPS、AAC 配置等
     * @return 0 表示成功，非 0 表示失败
     *
     * 构建并保存序列头，以便向服务器发送解码参数。
     */
    int SetMediaInfo(MediaInfo media_info);

    /**
     * @brief 打开 RTMP 推流地址并建立连接
     * @param url 推流地址，格式如 rtmp://域名/app/流名
     * @param msec 连接超时时间（毫秒）
     * @return 0 表示成功，-1 表示失败
     */
    int OpenUrl(std::string url, int msec);

    /**
     * @brief 推送 H.264 视频帧
     * @param data NAL 单元数据缓冲
     * @param size 数据长度（字节）
     * @return >=0 表示已发送字节数，<0 表示失败
     *
     * 首次遇到关键帧时，会先发送 SPS/PPS/AAC 序列头。
     */
    int PushVideoFrame(uint8_t *data, uint32_t size);

    /**
     * @brief 推送 AAC 音频帧
     * @param data AAC 原始帧数据，不包含 ADTS 头
     * @param size 数据长度（字节）
     * @return 0 表示已发送，<0 表示失败
     */
    int PushAudioFrame(uint8_t *data, uint32_t size);

    /**
     * @brief 关闭 RTMP 推流连接
     */
    void Close();

    /**
     * @brief 查询推流是否仍然连接中
     * @return true 表示已连接且未关闭，false 表示未连接或已关闭
     */
    bool IsConnected();

private:
    /**
     * @brief 私有构造函数，绑定事件循环
     */
    RtmpPublisher(EventLoop *event_loop);

    /**
     * @brief 判断当前 H.264 数据是否关键帧
     * @param data NAL 单元数据
     * @param size 数据长度
     * @return true 表示 I 帧（关键帧），false 表示非 I 帧
     */
    bool IsKeyFrame(uint8_t* data, uint32_t size);

    EventLoop *event_loop_ = nullptr;                    ///< 事件循环指针
    TaskScheduler  *task_scheduler_ = nullptr;           ///< 任务调度器指针
    std::shared_ptr<RtmpConnection> rtmp_conn_;          ///< RTMP 连接对象
    MediaInfo media_info_;                               ///< 媒体参数信息
    bool has_key_frame_ = false;                         ///< 是否已发送首个关键帧
    Timestamp timestamp_;                                ///< 推流时间戳生成器
    std::shared_ptr<char> avc_sequence_header_;          ///< H.264 SPS/PPS 序列头
    std::shared_ptr<char> aac_sequence_header_;          ///< AAC AudioSpecificConfig 序列头
    uint32_t avc_sequence_header_size_ = 0;              ///< SPS/PPS 序列头大小
    uint32_t aac_sequence_header_size_ = 0;              ///< AAC 序列头大小
};
#endif // RTMPPUBLISHER_H
