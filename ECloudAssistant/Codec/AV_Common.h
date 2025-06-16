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

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr  = std::shared_ptr<AVFrame>;

typedef struct VIDEOCONFIG
{
    quint32 width;
    quint32 height;
    quint32 bitrate;
    quint32 framerate;
    quint32 gop;
    AVPixelFormat format;
}VideoConfig;

typedef struct AUDIOCONFIG
{
    quint32 channels;
    quint32 samplerate;
    quint32 bitrate;
    AVSampleFormat format;
}AudioConfig;

struct AVConfig
{
    VideoConfig video;
    AudioConfig audio;
};

struct AVContext
{
public:
    //音频相关参数//
    int32_t audio_sample_rate;
    int32_t audio_channels_layout;
    AVRational audio_src_timebase;
    AVRational audio_dst_timebase;
    AVSampleFormat audio_fmt;
    double audioDuration;
    AVQueue<AVFramePtr> audio_queue_;
    //视频相关参数
    int32_t video_width;
    int32_t video_height;
    AVRational video_src_timebase;
    AVRational video_dst_timebase;
    AVPixelFormat video_fmt;
    double videoDuration;
    AVQueue<AVFramePtr> video_queue_;

    int avMediatype_ = 0;
};


class EncodBase
{
public:
    EncodBase():is_initialzed_(false),codec_(nullptr),codecContext_(nullptr){config_ = {};}
    virtual ~EncodBase(){if(codecContext_)avcodec_free_context(&codecContext_);}
    EncodBase(const EncodBase&) = delete;
    EncodBase& operator=(const EncodBase&) = delete;
public:
    virtual bool Open(AVConfig& config) = 0;
    virtual void Close() = 0;
    AVCodecContext* GetAVCodecContext() const
    {return codecContext_;}
protected:
    bool is_initialzed_ = false;
    AVConfig config_;
    AVCodec* codec_;
    AVCodecContext *codecContext_ = nullptr;
};

class DecodBase
{
public:
    DecodBase():is_initial_(false),video_index_(-1),audio_index_(-1),codec_(nullptr),codecCtx_(nullptr){config_ = {};}
    virtual ~DecodBase(){if(codecCtx_){avcodec_free_context(&codecCtx_);};}
    DecodBase(const DecodBase&) = delete;
    DecodBase& operator=(const DecodBase&) = delete;
    AVCodecContext* GetAVCodecContext() const
    {return codecCtx_;}
protected:
    bool is_initial_;
    std::mutex mutex_;
    qint32 video_index_;
    qint32 audio_index_;
    AVConfig config_;
    AVCodec* codec_;
    AVCodecContext *codecCtx_;
};

#endif //AV_COMMOEN_H
