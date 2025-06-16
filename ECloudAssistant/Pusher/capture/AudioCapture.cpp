#include "AudioCapture.h"
#include "AudioBuffer.h"
#include "WASAPICapture.h"
#include<QDebug>

AudioCapture::AudioCapture()
    :capture_(nullptr)
    ,audio_buffer_(nullptr)
{
    capture_.reset(new WASAPICapture());
}

AudioCapture::~AudioCapture()
{

}

bool AudioCapture::Init(uint32_t size)
{
    if(is_initailed_)
    {
        return true;
    }

    if(capture_->init() < 0)
    {
        return false;
    }
    else
    {
        WAVEFORMATEX* audoFmt = capture_->getAudioFormat();
        channels_ = audoFmt->nChannels;
        samplerate_ = audoFmt->nSamplesPerSec;
        bits_per_sample_ = audoFmt->wBitsPerSample;
    }

    //创建buffer
    audio_buffer_.reset(new AudioBuffer(size));
    //启动捕获器来捕获音频
    if(StartCapture() < 0)
    {
        return false;
    }
    is_initailed_ = true;
    return true;
}

void AudioCapture::Close()
{
    if(is_initailed_)
    {
        StopCapture();
        is_initailed_ = false;
    }
}

int AudioCapture::GetSamples()
{
    //从缓冲区获取当前有多少音频数据
    return audio_buffer_->size() * 8 / bits_per_sample_ / channels_;
}

int AudioCapture::Read(uint8_t *data, uint32_t samples)
{
    //从缓冲区读数据
    if(samples > this->GetSamples()) //说明数不足
    {
        return 0;
    }
    audio_buffer_->read((char*)data,samples * bits_per_sample_ / 8 * channels_);
    return samples;
}

int AudioCapture::StartCapture()
{
    //开始捕获
    capture_->setCallback([this](const WAVEFORMATEX *mixFormat, uint8_t *data, uint32_t samples){
        channels_ = mixFormat->nChannels;
        samplerate_ = mixFormat->nSamplesPerSec;
        bits_per_sample_ = mixFormat->wBitsPerSample;
        //将数据写入缓冲区
        audio_buffer_->write((char*)data,mixFormat->nBlockAlign * samples);
    });

    //音频需要清空来去缓存音频数据
    audio_buffer_->clear();
    //开始捕获音频
    if(capture_->start() < 0)
    {
        return -1;
    }
    is_stared_ = true;
    return 0;
}

int AudioCapture::StopCapture()
{
    capture_->stop();
    is_stared_ = false;
    return 0;
}
