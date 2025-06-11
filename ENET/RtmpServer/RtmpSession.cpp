/**
 * @file RtmpSession.cpp
 * @brief 实现 RtmpSession 类，用于管理直播会话中发布者和观看者连接、元数据和媒体数据的分发。
 */

#include "RtmpSession.h"
#include "RtmpSink.h"
#include "rtmp.h"
#include "RtmpConnection.h"

/**
 * @brief 默认构造函数
 */
RtmpSession::RtmpSession()
{
}

/**
 * @brief 默认析构函数
 */
RtmpSession::~RtmpSession()
{
}

/**
 * @brief 添加一个 RTMP Sink（发布者或观看者）到会话中
 * @param sink 要添加的连接接口
 */
void RtmpSession::AddSink(std::shared_ptr<RtmpSink> sink)
{
    std::lock_guard<std::mutex> lock(mutex_);            // 加锁保护并发访问
    rtmp_sinks_[sink->GetId()] = sink;                    // 将连接存入会话列表
    if (sink->IsPublisher())                             // 如果该连接是发布者
    {
        // 重置已存储的 Sequence Header，等待发布者重新推流
        avc_sequence_header_.reset();
        aac_sequence_header_.reset();
        avc_sequence_header_size_ = 0;
        aac_sequence_header_size_ = 0;
        has_publisher_ = true;
        publisher_ = sink;                                // 保存发布者弱引用
    }
}

/**
 * @brief 从会话中移除指定的 RTMP Sink
 * @param sink 要移除的连接接口
 */
void RtmpSession::RemoveSink(std::shared_ptr<RtmpSink> sink)
{
    std::lock_guard<std::mutex> lock(mutex_);            // 加锁保护并发访问
    if (sink->IsPublisher())                             // 如果移除的是发布者
    {
        // 清空发布者相关的元数据缓存
        avc_sequence_header_.reset();
        aac_sequence_header_.reset();
        avc_sequence_header_size_ = 0;
        aac_sequence_header_size_ = 0;
        has_publisher_ = false;
    }
    rtmp_sinks_.erase(sink->GetId());                    // 从映射中删除该连接
}

/**
 * @brief 获取当前会话中的在线客户端数量（包括发布者和观看者）
 * @return 在线客户端总数
 */
int RtmpSession::GetClients()
{
    std::lock_guard<std::mutex> lock(mutex_);
    int clients = 0;
    for (auto &entry : rtmp_sinks_)                       // 遍历所有连接
    {
        if (auto conn = entry.second.lock())              // 尝试提升弱引用
        {
            clients++;                                    // 计数有效连接
        }
    }
    return clients;
}

/**
 * @brief 向所有观看者发送元数据（AMF 对象），在正式推送媒体流前调用
 * @param metaData 键值对集合，包含流的属性信息
 */
void RtmpSession::SendMetaData(AmfObjects &metaData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = rtmp_sinks_.begin(); it != rtmp_sinks_.end();) 
    {
        if (auto conn = it->second.lock())              // 获取强引用判断是否在线，lock是一个线程安全的操作
        {
            if (conn->IsPlayer())                       // 仅发送给观看者
            {
                conn->SendMetaData(metaData);          // 调用接口发送元数据，这里的SendMetaData是RtmpSink的虚函数
            }
            ++it;
        }
        else
        {
            it = rtmp_sinks_.erase(it);                 // 清理已断开的连接
        }
    }
}

/**
 * @brief 向所有观看者分发音视频数据或 Sequence Header
 * @param type RTMP 媒体类型（RTMP_AUDIO/RTMP_VIDEO/SequenceHeader ID）
 * @param timestamp 帧时间戳（毫秒）
 * @param data 媒体数据缓冲智能指针
 * @param size 缓冲区大小（字节）
 */
void RtmpSession::SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> data, uint32_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = rtmp_sinks_.begin(); it != rtmp_sinks_.end();) 
    {
        if (auto conn = it->second.lock())              // 检查连接是否存活
        {
            if (conn->IsPlayer())                       // 仅对观看者推送
            {
                if (!conn->IsPlaying())                // 新进入的观看者先发送缓存的 Sequence Header
                {
                    // 先发送音频Header再发送视频Header，这里的SendMediaData是RtmpSink的虚函数
                    conn->SendMediaData(RTMP_AAC_SEQUENCE_HEADER, timestamp, aac_sequence_header_, aac_sequence_header_size_);
                    conn->SendMediaData(RTMP_AVC_SEQUENCE_HEADER, timestamp, avc_sequence_header_, avc_sequence_header_size_);
                }
                conn->SendMediaData(type, timestamp, data, size);  // 推送实际音视频帧
            }
            ++it;
        }
        else
        {
            it = rtmp_sinks_.erase(it);                   // 移除已断开的客户端
        }
    }
}

/**
 * @brief 获取当前会话的发布者连接
 * @return 如果发布者在线返回 RtmpConnection 智能指针，否则返回 nullptr
 */
std::shared_ptr<RtmpConnection> RtmpSession::GetPublisher()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto pub = publisher_.lock())                   // 尝试提升弱引用
    {
        //dynamic_pointer_cast<RtmpConnection> 是将 std::shared_ptr 转换为 RtmpConnection 类型
        return std::dynamic_pointer_cast<RtmpConnection>(pub);  // 转为 RtmpConnection 后返回
    }
    return nullptr;                                      // 发布者不存在或已断开
}
