#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H
#include <thread>
#include <cstdint>
#include <memory>

class AudioBuffer;
class WASAPICapture;
class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();
public:
    bool Init(uint32_t size = 20480);
    void Close();
    int GetSamples();
    int Read(uint8_t* data,uint32_t samples);
    inline bool CaptureStarted()const{return is_stared_;}
    inline uint32_t GetChannels()const {return channels_;}
    inline uint32_t GetSamplerate()const {return samplerate_;}
    inline uint32_t GetBitsPerSample()const {return bits_per_sample_;}
private:
    int StartCapture();
    int StopCapture();
    bool is_initailed_ = false;
    bool is_stared_ = false;
    uint32_t channels_ = 2;
    uint32_t samplerate_ = 48000;
    uint32_t bits_per_sample_ = 16;
    std::unique_ptr<WASAPICapture> capture_;
    std::unique_ptr<AudioBuffer> audio_buffer_;
};

#endif // AUDIOCAPTURE_H
