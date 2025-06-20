/**
 * @file AV_Common.h
 * @brief 定义音视频编码/解码基础数据结构、配置结构体及抽象基类
 * @date 2025-06-19
 */
#ifndef AV_COMMOEN_H
#define AV_COMMOEN_H

#include <QtGlobal>
#include <QDebug>
#include <memory>
#include <mutex>
#include "AV_Queue.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libavutil/error.h"
}

/**
 * @typedef AVPacketPtr
 * @brief AVPacket 智能指针类型，用于共享管理解复用或编码产生的数据包
 */
using AVPacketPtr = std::shared_ptr<AVPacket>;

/**
 * @typedef AVFramePtr
 * @brief AVFrame 智能指针类型，用于共享管理解码或录制产生的帧数据
 */
using AVFramePtr  = std::shared_ptr<AVFrame>;

/**
 * @struct VideoConfig
 * @brief 视频编码/解码配置参数
 */
typedef struct VIDEOCONFIG
{
    quint32 width;        ///< 视频帧宽度（像素）
    quint32 height;       ///< 视频帧高度（像素）
    quint32 bitrate;      ///< 视频码率（bps）
    quint32 framerate;    ///< 帧率（fps）
    quint32 gop;          ///< 关键帧间隔
    AVPixelFormat format; ///< 像素格式
} VideoConfig;

/**
 * @struct AudioConfig
 * @brief 音频编码/解码配置参数
 */
typedef struct AUDIOCONFIG
{
    quint32 channels;        ///< 音频通道数
    quint32 samplerate;      ///< 采样率（Hz）
    quint32 bitrate;         ///< 音频码率（bps）
    AVSampleFormat format;   ///< 采样格式
} AudioConfig;

/**
 * @struct AVConfig
 * @brief 音视频统一配置结构，包含 VideoConfig 与 AudioConfig
 */
struct AVConfig
{
    VideoConfig video; ///< 视频参数
    AudioConfig audio; ///< 音频参数
};

/**
 * @struct AVContext
 * @brief 音视频运行时上下文，包含解码队列、时基和格式信息
 */
struct AVContext
{
public:
    // 音频相关
    int32_t audio_sample_rate;           ///< 输入音频采样率
    int32_t audio_channels_layout;       ///< 声道布局
    AVRational audio_src_timebase;       ///< 原始音频时基
    AVRational audio_dst_timebase;       ///< 目标音频时基
    AVSampleFormat audio_fmt;            ///< 音频采样格式
    double audioDuration;                ///< 音频时长（秒）
    AVQueue<AVFramePtr> audio_queue_;    ///< 音频帧队列

    // 视频相关
    int32_t video_width;                 ///< 视频帧宽度
    int32_t video_height;                ///< 视频帧高度
    AVRational video_src_timebase;       ///< 原始视频时基
    AVRational video_dst_timebase;       ///< 目标视频时基
    AVPixelFormat video_fmt;             ///< 视频像素格式
    double videoDuration;                ///< 视频时长（秒）
    AVQueue<AVFramePtr> video_queue_;    ///< 视频帧队列

    int avMediatype_ = 0;                ///< 媒体类型标识（AVMEDIA_TYPE_AUDIO/VIDEO）
};

/**
 * @class EncodBase
 * @brief 音视频编码抽象基类，封装 AVCodecContext 管理和配置加载接口
 */
class EncodBase
{
public:
    /**
     * @brief 构造函数，初始化状态和上下文指针
     */
    EncodBase():is_initialzed_(false),codec_(nullptr),codecContext_(nullptr){config_ = {};}

    /**
     * @brief 析构函数，释放编码器上下文
     */
    virtual ~EncodBase(){ if(codecContext_) avcodec_free_context(&codecContext_); }

    EncodBase(const EncodBase&) = delete;
    EncodBase& operator=(const EncodBase&) = delete;

    /**
     * @brief 打开编码器并加载配置
     * @param config 编码参数
     * @return true 打开成功，false 失败
     */
    virtual bool Open(AVConfig& config) = 0;

    /**
     * @brief 关闭编码器并释放相关资源
     */
    virtual void Close() = 0;

    /**
     * @brief 获取底层 AVCodecContext 指针
     * @return AVCodecContext*
     */
    AVCodecContext* GetAVCodecContext() const { return codecContext_; }

protected:
    bool is_initialzed_;            ///< 是否已初始化
    AVConfig config_;               ///< 编码参数
    AVCodec* codec_;                ///< 编码器描述
    AVCodecContext* codecContext_;  ///< 编码上下文
};

/**
 * @class DecodBase
 * @brief 音视频解码抽象基类，封装 AVCodecContext 管理和解码流程接口
 */
class DecodBase
{
public:
    /**
     * @brief 构造函数，初始化解码器状态
     */
    DecodBase():is_initial_(false),video_index_(-1),audio_index_(-1),codec_(nullptr),codecCtx_(nullptr){config_ = {};}

    /**
     * @brief 析构函数，释放解码器上下文
     */
    virtual ~DecodBase(){ if(codecCtx_) avcodec_free_context(&codecCtx_); }

    DecodBase(const DecodBase&) = delete;
    DecodBase& operator=(const DecodBase&) = delete;

    /**
     * @brief 获取底层 AVCodecContext 指针
     * @return AVCodecContext*
     */
    AVCodecContext* GetAVCodecContext() const { return codecCtx_; }

protected:
    bool is_initial_;               ///< 是否已初始化
    std::mutex mutex_;              ///< 解码过程互斥保护
    qint32 video_index_;            ///< 视频流索引
    qint32 audio_index_;            ///< 音频流索引
    AVConfig config_;               ///< 解码参数
    AVCodec* codec_;                ///< 解码器描述
    AVCodecContext* codecCtx_;      ///< 解码上下文
};

#endif // AV_COMMOEN_H
