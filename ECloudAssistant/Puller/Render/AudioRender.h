#ifndef AUDIO_RENDER_H
#define AUDIO_RENDER_H
#include <QAudioOutput>
#include "AV_Common.h"

class AudioRender
{
public:
    AudioRender();
    ~AudioRender();
    inline bool IsInit(){return is_initail_;}
    //获取缓冲区pcm大小
    int AvailableBytes();
    bool InitAudio(int nChannels,int SampleRate,int nSampleSize);
    //播放音频
    void Write(AVFramePtr frame);
private:
    bool is_initail_ = false;
    int  nSampleSize_ = -1;
    int  volume_ = 50;
    //接入设备
    QAudioFormat  audioFmt_;
    QIODevice*    device_ = nullptr;
    QAudioOutput* audioOut_ = nullptr;
};

#endif
