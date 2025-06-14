/**
 * @file RtmpConnection.h
 * @brief 管理单个 TCP 连接上的 RTMP 会话，继承自 TcpConnection 并实现 RtmpSink 接口。
 *
 * 主要职责：
 *  - 完成 RTMP 握手流程
 *  - 接收并组装 Chunk 为完整消息
 *  - 使用 AMF0 编解码命令和数据
 *  - 处理 connect/createStream/publish/play/deleteStream 等命令
 *  - 将音视频数据分发给对应会话中的观众
 */

#pragma once

#include "../EdoyunNet/TcpConnection.h"
#include "amf.h"
#include "rtmp.h"
#include "RtmpSink.h"
#include "RtmpChunk.h"
#include "RtmpHandshake.h"

class RtmpServer;
class RtmpSession;
/**
 * @class RtmpConnection
 * @brief 继承自 TcpConnection 和 RtmpSink，实现 RTMP 客户端/服务器双向交互逻辑。
 */
class RtmpConnection : public TcpConnection, public RtmpSink
{
public:
    /**
     * @enum ConnectionState
     * @brief RTMP 会话当前阶段状态
     */
    enum ConnectionState
    {
        HANDSHAKE,          ///< 正在进行握手阶段
        START_CONNECT,      ///< 已接收到 connect 请求
        START_CREATE_STREAM,///< 已接收到 createStream 请求
        START_DELETE_STREAM,///< 已接收到 deleteStream 请求
        START_PLAY,         ///< 已接收到 play 请求
        START_PUBLISH,      ///< 已接收到 publish 请求
    };

    /**
     * @brief 构造函数，初始化握手状态并设置回调
     * @param rtmp_server RTMP 服务实例指针
     * @param scheduler   调度器用于事件回调
     * @param socket      底层 TCP 套接字
     */
    RtmpConnection(std::shared_ptr<RtmpServer> rtmp_server, TaskScheduler* scheduler, int socket);

    /**
     * @brief 析构函数，释放资源
     */
    virtual ~RtmpConnection();

    // --- RtmpSink 接口重写 ---
    virtual bool IsPlayer() override;     ///< 判断当前连接是否为观看者
    virtual bool IsPublisher() override;  ///< 判断当前连接是否为发布者
    virtual bool IsPlaying() override;    ///< 判断观看者是否正在播放
    virtual bool IsPublishing() override; ///< 判断发布者是否正在推流
    virtual uint32_t GetId() override;    ///< 获取唯一连接 ID

private:
    /**
     * @brief 处理读到的原始数据，先握手后分片处理
     * @param buffer 输入缓冲区
     * @return 是否成功
     */
    bool OnRead(BufferReader& buffer);

    /**
     * @brief 连接关闭回调，自动清理流（deleteStream）
     */
    void OnClose();

    /**
     * @brief 将缓冲区中的数据按 Chunk 解析并组装为完整 Message
     * @param buffer 输入缓冲区
     * @return 是否成功
     */
    bool HandleChunk(BufferReader& buffer);

    /**
     * @brief 处理完整的 RTMP 消息，根据类型分发到不同处理函数
     * @param rtmp_msg 完整消息结构
     * @return 是否成功
     */
    bool HandleMessage(RtmpMessage& rtmp_msg);

    // --- 各类消息处理 ---
    bool HandleInvoke(RtmpMessage& rtmp_msg);    ///< AMF 命令消息 invoke，启动消息
    bool HandleNotify(RtmpMessage& rtmp_msg);    ///< 数据消息 notify
    bool HandleAudio(RtmpMessage& rtmp_msg);     ///< 音频消息处理
    bool HandleVideo(RtmpMessage& rtmp_msg);     ///< 视频消息处理

    // --- 流程命令处理 ---
    bool HandleConnect();        ///< 处理 connect，建立连接
    bool HandleCreateStream();   ///< 处理 createStream，创建流
    bool HandlePublish();        ///< 处理 publish，开始推流
    bool HandlePlay();           ///< 处理 play，开始播放（拉流）
    bool HandleDeleteStream();   ///< 处理 deleteStream，删除流并清理会话

    // --- 控制消息发送 ---
    void SetPeerBandWidth();     ///< 发送带宽限制命令
    void SendAcknowlegement();   ///< 发送确认消息
    void SetChunkSize();         ///< 发送 chunk 大小设置命令

    // --- 消息发送工具 ---
    bool SendInvokeMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size);///< 封装 invoke 并发送，启动
    bool SendNotifyMessage(uint32_t csid, std::shared_ptr<char> payload, uint32_t payload_size);///< 封装 notify 并发送，通知
    bool IsKeyFrame(std::shared_ptr<char> data, uint32_t size);                                 ///< 判断 H264 关键帧
    void SendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg);                                 ///< 按 chunk 分片并发送

    // --- RtmpSink 接口实现 ---
    virtual bool SendMetaData(AmfObjects metaData) override; ///< RtmpSink 元数据发送
    virtual bool SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size) override;///< RtmpSink 媒体数据发送

private:
    ConnectionState state_;                       ///< 当前 RTMP 流程状态
    std::shared_ptr<RtmpHandshake> handshake_;    ///< 握手状态机
    std::shared_ptr<RtmpChunk> rtmp_chunk_;      ///< Chunk 解析/构建器

    std::weak_ptr<RtmpServer> rtmp_server_;      ///< RTMP 服务弱引用
    std::weak_ptr<RtmpSession> rtmp_session_;    ///< 当前会话弱引用

    uint32_t peer_width_;            ///< 对等方可接收带宽
    uint32_t ackonwledgement_size_;  ///< 确认窗口大小
    uint32_t max_chunk_size_;        ///< 最大 chunk 大小
    uint32_t stream_id_;             ///< 当前 stream id

    AmfObjects meta_data_;           ///< 缓存的元数据
    AmfDecoder amf_decoder_;         ///< AMF 解码器
    AmfEncoder amf_encoder_;         ///< AMF 编码器

    bool is_playing_;                ///< 播放者播放状态
    bool is_publishing_;             ///< 发布者推流状态

    std::string app_;                ///< 应用名称
    std::string stream_name_;        ///< 流名称
    std::string stream_path_;        ///< 完整流路径

    bool has_key_frame;              ///< 是否已收到关键帧，用于延迟推流

    // AVC/AAC 序列头缓存
    std::shared_ptr<char> avc_sequence_header_; ///< 视频序列头
    std::shared_ptr<char> aac_sequence_header_; ///< 音频序列头
    uint32_t avc_sequence_header_size_;         ///< 视频序列头长度
    uint32_t aac_sequence_header_szie_;         ///< 音频序列头长度
};