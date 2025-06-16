#ifndef H264_DECODER_H
#define H264_DECODER_H
#include <QThread>
#include "AV_Common.h"

class VideoConverter;
class H264_Decoder : public QThread ,public DecodBase
{
    Q_OBJECT
public:
    H264_Decoder(AVContext* ac,QObject* parent = nullptr);
    ~H264_Decoder();
    int  Open(const AVCodecParameters* codecParamer);
    inline bool isFull(){return video_queue_.size() > 10;}
    inline void put_packet(const AVPacketPtr packet){video_queue_.push(packet);}
protected:
    void Close();
    virtual void run()override;
private:
    bool quit_ = false;
    AVFramePtr yuv_frame_ = nullptr;
    AVQueue<AVPacketPtr> video_queue_;
    AVContext*    avContext_ = nullptr;
    std::unique_ptr<VideoConverter> videoConver_;
};

#endif // H264_DECODER_H
