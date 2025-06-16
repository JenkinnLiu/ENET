#ifndef RTMPCONNECTION
#define RTMPCONNECTION
#include "TcpConnection.h"
#include "amf.h"
#include "rtmp.h"
#include "RtmpChunk.h"
#include "RtmpHandshake.h"

class RtmpPublisher;
enum ConnectionState
{
    HANDSHAKE,
    START_CONNECT,
    START_CREATE_STREAM,
    START_DELETE_STREAM,
    START_PUBLISH,
};

class RtmpPublisher;
class RtmpConnection : public TcpConnection
{
public:
    RtmpConnection(std::shared_ptr<RtmpPublisher> publisher, TaskScheduler* scheduler, int sockfd);
    virtual ~RtmpConnection();

    bool OnRead(BufferReader& buffer);
    void OnClose();

    bool HandleChunk(BufferReader& buffer);
    bool HandleMessage(RtmpMessage& rtmp_msg);
    bool HandleInvoke(RtmpMessage& rtmp_msg);

    bool Handshake();
    bool Connect();
    bool CretaeStream();
    bool Publish();
    bool DeleteStream();

    bool HandleResult(RtmpMessage& rtmp_msg);
    bool HandleOnStatus(RtmpMessage& rtmp_msg);
    void SetChunkSize();

    bool SendInvokeMsg(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size);
    bool SendNotifyMsg(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size);
    bool IsKeyFrame(std::shared_ptr<char> data, uint32_t size);
    bool SendVideoData(uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size);
    bool SendAudioData(uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size);
    void SendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg);

private:
    RtmpConnection(TaskScheduler *scheduler, int sockfd, Rtmp* rtmp);
    std::string app_;
    std::string stream_name_;
    std::string stream_path_;

    uint32_t number_ = 0;
    uint32_t stream_id_ = 0;
    uint32_t max_chunk_size_ = 128;
    uint32_t avc_sequence_header_size_ = 0;
    uint32_t aac_sequence_header_size_ = 0;

    bool is_publishing_ = false;
    bool has_key_frame_ = false;

    ConnectionState state_;
    TaskScheduler *task_scheduler_;

    std::weak_ptr<RtmpPublisher> rtmp_publisher_;

    std::unique_ptr<AmfDecoder> amf_decoder_ = nullptr;
    std::unique_ptr<AmfEncoder> amf_encoder_ = nullptr;
    std::unique_ptr<RtmpChunk> rtmp_chunk_ = nullptr;
    std::unique_ptr<RtmpHandshake> handshake_ = nullptr;

    std::shared_ptr<char> avc_sequence_header_ = nullptr;
    std::shared_ptr<char> aac_sequence_header_ = nullptr;
};
#endif
