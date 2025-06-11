/**
 * @file RtmpSink.h
 * @brief 定义发送 RTMP 消息的抽象接口 RtmpSink。
 *
 * RtmpSink 提供统一的 SendMetaData 和 SendMediaData 接口，
 * 以及玩家/发布者身份判断和唯一标识获取方法。
 */

#ifndef _RTMPSINK_H_
#define _RTMPSINK_H_
#include <cstdint>
#include <memory>
#include "amf.h"

/**
 * @class RtmpSink
 * @brief RTMP 消息发送者抽象接口，可以是 Publisher 或 Player。
 */
class RtmpSink
{
public:
    /**
     * @brief 构造函数
     */
    RtmpSink(){}
    /**
     * @brief 虚析构函数，允许子类清理资源
     */
    virtual ~RtmpSink(){}

    /**
     * @brief 发送元数据消息（AMF 对象），在音视频正式推送前使用
     * @param metaData 键值对集合，包含流的属性信息
     * @return true 表示发送成功或忽略
     */
    virtual bool SendMetaData(AmfObjects metaData) {return true;}

    /**
     * @brief 发送音视频数据或 Sequence Header
     * @param type   RTMP 媒体类型（比如 RTMP_AUDIO 或 RTMP_VIDEO 等）
     * @param timestamp 时间戳（单位：毫秒）
     * @param payload  数据缓冲智能指针
     * @param payload_size 缓冲大小（字节）
     * @return true 表示发送成功
     */
    virtual bool SendMediaData(uint8_t type,uint64_t timestamp,std::shared_ptr<char> payload,uint32_t payload_size) = 0;

    /**
     * @brief 判断是否为播放端（观看者）
     * @return true 如果是观看者
     */
    virtual bool IsPlayer() {return false;}
    /**
     * @brief 判断是否为发布端（推流者）
     * @return true 如果是推流者
     */
    virtual bool IsPublisher() {return false;}
    /**
     * @brief 判断播放器是否已接收过元数据
     * @return true 如果正在播放
     */
    virtual bool IsPlaying() {return false;}
    /**
     * @brief 判断发布者是否已完成推流初始化
     * @return true 如果已开始推流
     */
    virtual bool IsPublishing() {return false;}

    /**
     * @brief 获取此连接的唯一 ID（用于会话管理）
     * @return 整型 ID
     */
    virtual uint32_t GetId() = 0;

};
#endif