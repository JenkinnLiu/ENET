#ifndef VIDEO_CONVERTER_H
#define VIDEO_CONVERTER_H
#include "AV_Common.h"
struct SwsContext;
class VideoConverter
{
public:
    VideoConverter();
    virtual ~VideoConverter();
    VideoConverter(const VideoConverter&) = delete;
    VideoConverter& operator=(const VideoConverter&) = delete;
public:
    bool Open(qint32 in_width,qint32 in_height,AVPixelFormat in_format,
              qint32 out_width,qint32 out_height,AVPixelFormat out_format);
    void Close();

    qint32 Convert(AVFramePtr in_frame,AVFramePtr& out_frame);
private:
    qint32 width_;
    qint32 height_;
    AVPixelFormat format_;
    SwsContext* swsContext_;
};
#endif // VIDEO_CONVERTER_H
