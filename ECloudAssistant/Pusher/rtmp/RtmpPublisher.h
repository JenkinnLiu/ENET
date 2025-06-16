#ifndef RTMPPUBLISHER_H
#define RTMPPUBLISHER_H
#include "rtmp.h"
#include "EventLoop.h"
#include "TimeStamp.h"
#include "RtmpConnection.h"

class RtmpPublisher : public Rtmp, public std::enable_shared_from_this<RtmpPublisher>
{
public:
    static std::shared_ptr<RtmpPublisher> Create(EventLoop* loop);
    ~RtmpPublisher();
    int  SetMediaInfo(MediaInfo media_info);
    int  OpenUrl(std::string url, int msec);
    int  PushVideoFrame(uint8_t *data, uint32_t size);
    int  PushAudioFrame(uint8_t *data, uint32_t size);
    void Close();
    bool IsConnected();
private:
    RtmpPublisher(EventLoop *event_loop);
    bool IsKeyFrame(uint8_t* data, uint32_t size);
    EventLoop *event_loop_ = nullptr;
    TaskScheduler  *task_scheduler_ = nullptr;
    std::shared_ptr<RtmpConnection> rtmp_conn_;
    MediaInfo media_info_;
    bool has_key_frame_ = false;
    Timestamp timestamp_;
    std::shared_ptr<char> avc_sequence_header_;
    std::shared_ptr<char> aac_sequence_header_;
    uint32_t avc_sequence_header_size_ = 0;
    uint32_t aac_sequence_header_size_ = 0;
};
#endif // RTMPPUBLISHER_H
