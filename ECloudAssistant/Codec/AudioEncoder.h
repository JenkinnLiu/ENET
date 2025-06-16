#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H
#include "AV_Common.h"
class AudioResampler;
class AudioEncoder : public EncodBase
{
public:
    AudioEncoder();
    AudioEncoder(const AudioEncoder&) = delete;
    AudioEncoder& operator=(const AudioEncoder&) = delete;
    ~AudioEncoder();
public:
    virtual bool Open(AVConfig& config) override;
    virtual void Close() override;
    uint32_t     GetFrameSamples();
    AVPacketPtr  Encode(const uint8_t *pcm, int samples);
private:
    int64_t pts_ = 0;
    std::unique_ptr<AudioResampler> audio_resampler_;
};

#endif // AUDIO_ENCODER_H
