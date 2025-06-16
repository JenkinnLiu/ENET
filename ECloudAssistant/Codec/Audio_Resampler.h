#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H
#include "AV_Common.h"
struct SwrContext;
class AudioResampler
{
public:
    AudioResampler();
    AudioResampler(const AudioResampler&) = delete;
    AudioResampler& operator=(const AudioResampler&) = delete;
    ~AudioResampler();
public:
    void Close();
    int  Convert(AVFramePtr in_frame,AVFramePtr& out_frame);
    bool Open(int in_samplerate,int in_channels,AVSampleFormat in_format,
              int out_samplerate, int out_channels, AVSampleFormat out_format);
private:
    SwrContext* swr_context_;
    int in_samplerate_ = 0;
    int in_channels_ = 0;
    int in_bits_per_sample_ = 0;
    AVSampleFormat in_format_ = AV_SAMPLE_FMT_NONE;

    int out_samplerate_ = 0;
    int out_channels_ = 0;
    int out_bits_per_sample_ = 0;
    AVSampleFormat out_format_ = AV_SAMPLE_FMT_NONE;
};

#endif // AUDIO_RESAMPLER_H
