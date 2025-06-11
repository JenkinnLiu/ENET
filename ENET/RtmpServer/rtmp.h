/**
 * @file rtmp.h
 * @brief 定义 RTMP 协议相关常量及基础 Rtmp 类接口。
 *
 * 本文件提供：
 *  - RTMP 协议版本号、命令消息类型、Chunk 类型等常量定义
 *  - 默认 Chunk 大小、带宽及确认大小参数设置
 *  - Rtmp 类，包含 Chunk 配置及用户 RTMP URL 解析功能
 */

#ifndef _RTMP_H_
#define _RTMP_H_
#include <cstdint>
#include <cstring>


// --- RTMP 消息类型（MessageTypeID） -----------------------------------------
/** RTMP 协议版本号 */
static const int RTMP_VERSION = 0x03;
/** 设置 Chunk 大小命令 */
static const int RTMP_SET_CHUNK_SIZE = 0x01;
/** 中止消息 */
static const int RTMP_ABORT_MESSAGE = 0x02;
/** 确认消息 */
static const int RTMP_ACK = 0x03;
/** 确认消息大小 */
static const int RTMP_ACK_SIZE = 0x05;
/** 带宽消息大小 */
static const int RTMP_BANDWIDTH_SIZE = 0x06;
/** 音频消息类型 */
static const int RTMP_AUDIO = 0x08;
/** 视频消息类型 */
static const int RTMP_VIDEO = 0x09;
/** 通知消息（Data Message） */
static const int RTMP_NOTIFY = 0x12;
/** 调用消息（Command Message） */
static const int RTMP_INVOKE = 0x14;

// --- Chunk 头格式类型（FMT 类型） ---------------------------------------------
/** 基本头类型 0 (完整消息头) */
static const int RTMP_CHUNK_TYPE_0 = 0;
/** 基本头类型 1 (时间戳、长度、类型ID) */
static const int RTMP_CHUNK_TYPE_1 = 1;
/** 基本头类型 2 (仅时间戳偏移) */
static const int RTMP_CHUNK_TYPE_2 = 2;
/** 基本头类型 3 (仅基本头) */
static const int RTMP_CHUNK_TYPE_3 = 3;

// --- RTMP 默认通道 ID ---------------------------------------------------------
/** 控制消息通道 */
static const int RTMP_CHUNK_CONTROL_ID = 2;
/** 调用消息通道 */
static const int RTMP_CHUNK_INVOKE_ID = 3;
/** 音频消息通道 */
static const int RTMP_CHUNK_AUDIO_ID = 4;
/** 视频消息通道 */
static const int RTMP_CHUNK_VIDEO_ID = 5;
/** 数据消息通道 */
static const int RTMP_CHUNK_DATA_ID = 6;

// --- 编解码类型 ---------------------------------------------------------------
/** H.264 视频 Codec ID */
static const int RTMP_CODEC_ID_H264 = 7;
/** AAC 音频 Codec ID */
static const int RTMP_CODEC_ID_AAC = 10;

// --- 元数据类型 ID -------------------------------------------------------------
/** AVC Sequence Header (视频元数据) */
static const int RTMP_AVC_SEQUENCE_HEADER = 0x18;
/** AAC Sequence Header (音频元数据) */
static const int RTMP_AAC_SEQUENCE_HEADER = 0x19;

/**
 * @class Rtmp
 * @brief RTMP 协议处理基类，提供 Chunk 配置及 URL 解析功能
 */
class Rtmp
{
public:
    virtual ~Rtmp() {};

    /**
     * @brief 设置最大 Chunk 大小
     * @param size 字节数（范围 1～60000）
     */
    void SetChunkSize(uint32_t size)
    {
        if (size > 0 && size <= 60000) {
            max_chunk_size_ = size;
        }
    }

    /**
     * @brief 设置对等方（Peer）带宽限制
     * @param size 带宽大小（字节）
     */
    void SetPeerBandwidth(uint32_t size)
    { peer_bandwidth_ = size; }

    /**
     * @brief 获取当前最大 Chunk 大小
     * @return Chunk 大小（字节）
     */
    uint32_t GetChunkSize() const
    { return max_chunk_size_; }

    /**
     * @brief 获取当前确认窗口大小
     * @return 确认消息大小（字节）
     */
    uint32_t GetAcknowledgementSize() const
    { return acknowledgement_size_; }

    /**
     * @brief 获取当前对等方带宽设置
     * @return 带宽大小（字节）
     */
    uint32_t GetPeerBandwidth() const
    { return peer_bandwidth_; }

    /**
     * @brief 解析 RTMP URL (rtmp://host[:port]/app/streamName)
     * @param url 完整 RTMP URL 字符串
     * @return 0 成功，-1 失败
     */
    virtual int ParseRtmpUrl(std::string url)
    {
        char ip[100] = { 0 };
        char streamPath[500] = { 0 };
        char app[100] = { 0 };
        char streamName[400] = { 0 };
        uint16_t port = 0;

        if (strstr(url.c_str(), "rtmp://") == nullptr) {
            return -1;
        }

#if defined(__linux) || defined(__linux__)
        if (sscanf(url.c_str() + 7, "%[^:]:%hu/%s", ip, &port, streamPath) == 3)
#elif defined(WIN32) || defined(_WIN32)
        if (sscanf_s(url.c_str() + 7, "%[^:]:%hu/%s", ip, 100, &port, streamPath, 500) == 3)
#endif
        {
            port_ = port;
        }
#if defined(__linux) || defined(__linux__)
        else if (sscanf(url.c_str() + 7, "%[^/]/%s", ip, streamPath) == 2)
#elif defined(WIN32) || defined(_WIN32)
        else if (sscanf_s(url.c_str() + 7, "%[^/]/%s", ip, 100, streamPath, 500) == 2)
#endif
        {
            port_ = 1935;
        }
        else {
            return -1;
        }

        ip_ = ip;
        stream_path_ += "/";
        stream_path_ += streamPath;

#if defined(__linux) || defined(__linux__)
        if (sscanf(stream_path_.c_str(), "/%[^/]/%s", app, streamName) != 2)
#elif defined(WIN32) || defined(_WIN32)
        if (sscanf_s(stream_path_.c_str(), "/%[^/]/%s", app, 100, streamName, 400) != 2)
#endif
        {
            return -1;
        }

        app_ = app;
        stream_name_ = streamName;
        return 0;
    }

    /** @brief 获取完整流路径 ("/app/streamName") */
    std::string GetStreamPath() const
    { return stream_path_; }
    /** @brief 获取应用名称 (app) */
    std::string GetApp() const
    { return app_; }
    /** @brief 获取流名称 (streamName) */
    std::string GetStreamName() const
    { return stream_name_; }

    /** @brief 默认 RTMP 端口 */
    uint16_t port_ = 1935;
    /** @brief 服务器或客户端 IP 地址 */
    std::string ip_;
    /** @brief 应用名称 */
    std::string app_;
    /** @brief 流名称 */
    std::string stream_name_;
    /** @brief 完整流路径，形如 "/app/streamName" */
    std::string stream_path_;

    /** @brief 对等方最大带宽 */
    uint32_t peer_bandwidth_ = 5000000;
    /** @brief 确认窗口大小 */
    uint32_t acknowledgement_size_ = 5000000;
    /** @brief 最大 Chunk 大小 */
    uint32_t max_chunk_size_ = 128;
};
#endif