/**
 * @file AudioEncoder.cpp
 * @brief 实现 AudioEncoder 类，包括初始化编码器、重采样、PCM 到 AAC 编码及释放资源等功能
 * @date 2025-06-19
 */

#include "AudioEncoder.h"
#include "Audio_Resampler.h"

/**
 * @brief 构造函数，初始化重采样器指针为空
 */
AudioEncoder::AudioEncoder()
    : audio_resampler_(nullptr)
{
}

/**
 * @brief 析构函数，确保所有资源关闭并释放
 */
AudioEncoder::~AudioEncoder()
{
    Close();
}

/**
 * @brief 打开 AAC 编码器并初始化重采样器
 * @param config 音频配置参数，包含采样率、通道等
 * @return true 打开并初始化成功，false 失败
 */
bool AudioEncoder::Open(AVConfig &config)
{
    if(is_initialzed_)
        return false;

    config_ = config;
    // 查找 AAC 编码器
    codec_ = const_cast<AVCodec*>(avcodec_find_encoder(AV_CODEC_ID_AAC));
    if(!codec_)
        return false;

    // 分配编码上下文
    codecContext_ = avcodec_alloc_context3(codec_);
    if(!codecContext_)
        return false;

    // 配置编码参数
    codecContext_->sample_rate    = config.audio.samplerate;// 设置采样率
    codecContext_->sample_fmt     = AV_SAMPLE_FMT_FLTP;// 设置输出格式为浮点 PCM
    codecContext_->channels       = config.audio.channels;// 设置声道数
    codecContext_->channel_layout = av_get_default_channel_layout(config.audio.channels);// 设置声道布局
    codecContext_->bit_rate       = config.audio.bitrate;// 设置码率
    codecContext_->flags         |= AV_CODEC_FLAG_GLOBAL_HEADER;// 设置全局头部标志

    if(avcodec_open2(codecContext_, codec_, NULL) < 0)// 打开编码器
        return false;

    // 创建并初始化重采样器
    audio_resampler_.reset(new AudioResampler());
    if(!audio_resampler_->Open(
            config.audio.samplerate,
            config.audio.channels,
            config.audio.format,
            config.audio.samplerate,
            config.audio.channels,
            AV_SAMPLE_FMT_FLTP))
    {
        return false;
    }

    is_initialzed_ = true;
    return true;
}

/**
 * @brief 关闭编码器并释放重采样器资源
 */
void AudioEncoder::Close()
{
    if(audio_resampler_)
    {
        audio_resampler_->Close();
        audio_resampler_.reset();
    }
    EncodBase::Close();
}

/**
 * @brief 获取编码器的帧样本数，用于数据分包
 * @return 单帧样本数，编码器初始化后有效
 */
uint32_t AudioEncoder::GetFrameSamples()
{
    if(is_initialzed_)
        return codecContext_->frame_size;
    return 0;
}

/**
 * @brief 对输入的 PCM 数据进行重采样并编码为 AAC
 * @param pcm     输入 PCM 原始数据指针
 * @param samples PCM 样本帧数
 * @return 返回编码后的 AVPacketPtr，失败返回 nullptr
 */
AVPacketPtr AudioEncoder::Encode(const uint8_t *pcm, int samples)
{
    // 1. 准备输入帧
    AVFramePtr in_frame(av_frame_alloc(),[](AVFrame* ptr){av_frame_free(&ptr);});
    in_frame->sample_rate    = codecContext_->sample_rate;// 设置采样率
    in_frame->format         = config_.audio.format;// 设置输入格式
    in_frame->channels       = codecContext_->channels;//  设置声道数
    in_frame->channel_layout = codecContext_->channel_layout;// 设置声道布局
    in_frame->nb_samples     = samples;// 设置样本数
    in_frame->pts            = av_rescale_q(pts_,{1,codecContext_->sample_rate},codecContext_->time_base);// 设置时间戳
    pts_ += in_frame->nb_samples;// 更新时间戳

    if(av_frame_get_buffer(in_frame.get(), 0) < 0)// 分配输入帧的缓冲区
        return nullptr;

    int bytes_per_sample = av_get_bytes_per_sample(config_.audio.format);// 获取每个样本的字节数
    if(bytes_per_sample <= 0)
        return nullptr;
    memcpy(in_frame->data[0], pcm, bytes_per_sample * samples * in_frame->channels);// 将 PCM 数据复制到输入帧的缓冲区

    // 2. 重采样
    AVFramePtr fltp_frame;
    if(audio_resampler_->Convert(in_frame, fltp_frame) <= 0)// 执行重采样，将 PCM 转换为浮点格式
        return nullptr;

    // 3. 送入编码器
    if(avcodec_send_frame(codecContext_, fltp_frame.get()) < 0)// 发送重采样后的帧到编码器
        return nullptr;

    // 4. 接收编码包
    AVPacketPtr packet(av_packet_alloc(),[](AVPacket* ptr){av_packet_free(&ptr);});// 分配 AVPacket 用于接收编码后的数据
    av_init_packet(packet.get());
    int ret = avcodec_receive_packet(codecContext_, packet.get());// 从编码器接收编码后的数据包
    if(ret < 0)
        return nullptr;

    return packet;
}
