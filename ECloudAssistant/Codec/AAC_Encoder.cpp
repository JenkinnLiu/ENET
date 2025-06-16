#include "AAC_Encoder.h"
#include "AudioEncoder.h"

AACEncoder::AACEncoder()
{
    AAC_encoder_.reset(new AudioEncoder());
}

AACEncoder::~AACEncoder()
{
    Close();
}

bool AACEncoder::Open(int samplerate, int channels, int format, int bitrate_kbps)
{
    //初始化这个AAC编码
    if(AAC_encoder_->GetAVCodecContext())
    {
        //已经初始化完毕
        return false;
    }
    //没有初始化
    AVConfig encoder_config;
    encoder_config.audio.samplerate = samplerate;
    encoder_config.audio.channels = channels;
    encoder_config.audio.bitrate = bitrate_kbps;
    encoder_config.audio.format = (AVSampleFormat)format;
    //初始化这个编码器
    if(!AAC_encoder_->Open(encoder_config))
    {
        return false;
    }
    return true;
}

void AACEncoder::Close()
{
    channel_ = 0;
    bitrate_ = 0;
    samplerate_ = 0;
    format_ = AV_SAMPLE_FMT_NONE;
    if(AAC_encoder_)
    {
        AAC_encoder_->Close();
        AAC_encoder_.reset();
        AAC_encoder_ = nullptr;
    }
}

int AACEncoder::GetFrames()
{
    if(!AAC_encoder_->GetAVCodecContext())
    {
        return -1;
    }
    return AAC_encoder_->GetFrameSamples();
}

int AACEncoder::GetSpecificConfig(uint8_t *buf, int max_buf_size)
{
    //再是获取编码器参数
    //先要获取编码器上下文
    AVCodecContext* codecContxt = AAC_encoder_->GetAVCodecContext();
    if(!codecContxt)
    {
        return -1;
    }
    //还需要判断这个上下文参数大小跟这个输入大小对比
    if(max_buf_size < codecContxt->extradata_size)
    {
        return -1;
    }
    //再将参数拷贝出去
    memcpy(buf,codecContxt->extradata,codecContxt->extradata_size);
    return codecContxt->extradata_size;
}

AVPacketPtr AACEncoder::Encode(const uint8_t *pcm, int samples)
{
    //编码AAC
    //先要判断aac编码器是否存在
    if(!AAC_encoder_->GetAVCodecContext())
    {
        return nullptr;
    }
    return AAC_encoder_->Encode(pcm,samples);
}
