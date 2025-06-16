#ifndef GDISCREENCAPTURE_H
#define GDISCREENCAPTURE_H
#include <QThread>
#include <memory>
#include <mutex>
struct AVFrame;
struct AVPacket;
struct AVInputFormat;
struct AVCodecContext;
struct AVFormatContext;
using FrameContainer = std::vector<quint8>;

class GDIScreenCapture : public QThread
{
public:
    GDIScreenCapture();
    GDIScreenCapture(const GDIScreenCapture&) = delete;
    GDIScreenCapture& operator=(const GDIScreenCapture&) = delete;
    virtual ~GDIScreenCapture();
public:
    virtual quint32 GetWidth() const;
    virtual quint32 GetHeight() const;
    virtual bool Init(qint64 display_index = 0);
    virtual bool Close();
    virtual bool CaptureFrame(FrameContainer& rgba,quint32& width,quint32& height);
protected:
    virtual void run() override;
private:
    void StopCapture();
    bool GetOneFrame();
    bool Decode(AVFrame* av_frame,AVPacket* av_packet);
private:
    using framPtr = std::shared_ptr<quint8>;
    bool    stop_;
    bool    is_initialzed_;
    quint32 frame_size_;
    framPtr rgba_frame_;
    quint32 width_;
    quint32 height_;
    qint64  video_index_;
    qint64  framerate_;
    std::mutex mutex_;
    AVInputFormat* input_format_;
    AVCodecContext* codec_context_;
    AVFormatContext* format_context_;
    std::shared_ptr<AVFrame> av_frame_;
    std::shared_ptr<AVPacket> av_packet_;
};
#endif // GDISCREENCAPTURE_H
