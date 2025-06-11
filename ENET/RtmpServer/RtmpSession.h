#include <memory>
#include <mutex>
#include "amf.h"

/**
 * @class RtmpSession
 * @brief 管理一个直播会话，维护所有发布者和观看者连接，并支持元数据和流数据分发。
 */
class RtmpSession
{
public:
    using Ptr = std::shared_ptr<RtmpSession>;

    /**
     * @brief 构造一个 RtmpSession 实例
     */
    RtmpSession();
    /**
     * @brief 析构函数，清理会话资源
     */
    virtual ~RtmpSession();

    /**
     * @brief 设置 H264 AVC Sequence Header，用于后续视频流推送
     * @param avcSequenceHeader AVC Sequence Header 数据
     * @param avcSequenceHeaderSize 数据长度（字节）
     */
    void SetAvcSequenceHeader(std::shared_ptr<char> avcSequenceHeader,uint32_t avcSequenceHeaderSize)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        avc_sequence_header_ = avcSequenceHeader;
        avc_sequence_header_size_ = avcSequenceHeaderSize;
    }

    /**
     * @brief 设置 AAC Sequence Header，用于后续音频流推送
     * @param aacSequenceHeader AAC Sequence Header 数据
     * @param aacSequenceHeaderSize 数据长度（字节）
     */
    void SetAacSequenceHeader(std::shared_ptr<char> aacSequenceHeader,uint32_t aacSequenceHeaderSize)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        aac_sequence_header_ = aacSequenceHeader;
        aac_sequence_header_size_ = aacSequenceHeaderSize;
    }

    /**
     * @brief 添加一个新的 RTMP Sink（客户端或发布者）到会话
     * @param sink 客户端连接接口
     */
    void AddSink(std::shared_ptr<RtmpSink> sink);

    /**
     * @brief 从会话中移除一个 RTMP Sink
     * @param sink 客户端连接接口
     */
    void RemoveSink(std::shared_ptr<RtmpSink> sink);

    /**
     * @brief 获取当前会话中在线的客户端数量（发布者+观看者）
     * @return 在线客户端总数
     */
    int GetClients();

    /**
     * @brief 向所有观看者发送元数据（AmfObjects）
     * @param metaData 元数据键值对集合
     */
    void SendMetaData(AmfObjects& metaData);

    /**
     * @brief 向会话中的所有观看者发送音视频数据
     * @param type RTMP 媒体类型（音频或视频或 SequenceHeader）
     * @param timestamp 时间戳（毫秒）
     * @param data 媒体数据负载
     * @param size 数据长度（字节）
     */
    void SendMediaData(uint8_t type,uint64_t timestamp,std::shared_ptr<char> data,uint32_t size);

    /**
     * @brief 获取当前会话的发布者连接，如果不存在则返回 nullptr
     * @return 发布者 RtmpConnection 智能指针或 nullptr
     */
    std::shared_ptr<RtmpConnection> GetPublisher();
private:
    std::mutex mutex_;               /**< 保护 rtmp_sinks_ 及 sequence header 访问的互斥量 */
    bool has_publisher_ = false;     /**< 标志是否已有发布者 */
    std::weak_ptr<RtmpSink> publisher_; /**< 发布者连接的弱引用 */
    std::unordered_map<int,std::weak_ptr<RtmpSink>> rtmp_sinks_; /**< 会话中所有客户端连接集合 */

    std::shared_ptr<char> avc_sequence_header_;   /**< 缓存的视频 Sequence Header 数据 */
    std::shared_ptr<char> aac_sequence_header_;   /**< 缓存的音频 Sequence Header 数据 */
    uint32_t avc_sequence_header_size_ = 0;       /**< 视频序列头长度 */
    uint32_t aac_sequence_header_size_ = 0;       /**< 音频序列头长度 */
};