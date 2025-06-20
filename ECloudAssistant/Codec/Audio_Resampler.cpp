/**
 * @file Audio_Resampler.cpp
 * @brief 实现 AudioResampler 类，负责音频采样率、通道和格式的重采样功能
 * @date 2025-06-19
 */
#include "Audio_Resampler.h"

extern "C"
{
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/samplefmt.h>
}

/**
 * @brief 构造函数，初始化 SwrContext 指针为空
 */
AudioResampler::AudioResampler()
    : swr_context_(nullptr)
{
}

/**
 * @brief 析构函数，调用 Close 释放重采样资源
 */
AudioResampler::~AudioResampler()
{
    Close();
}

/**
 * @brief 关闭重采样器，释放并清理 SwrContext 资源
 */
void AudioResampler::Close()
{
    if (swr_context_)
    {
        if (swr_is_initialized(swr_context_))
        {
            swr_close(swr_context_);
        }
        swr_context_ = nullptr;
    }
}

/**
 * @brief 将输入音频帧转换为输出格式的音频帧
 * @param in_frame 输入的 AVFramePtr，包含源格式采样数据
 * @param out_frame 输出的 AVFramePtr 引用，将返回重采样后的数据
 * @return 返回重采样后的样本数，负值表示失败
 */
int AudioResampler::Convert(AVFramePtr in_frame, AVFramePtr &out_frame)
{
    if (!swr_context_)
    {
        return -1;
    }

    // 申请空间并配置输出帧
    out_frame.reset(av_frame_alloc(), [](AVFrame *ptr) { av_frame_free(&ptr); });
    out_frame->sample_rate = out_samplerate_;
    out_frame->format = out_format_;
    out_frame->channels = out_channels_;
    
    // 计算重采样后的样本数
    int64_t delay = swr_get_delay(swr_context_, in_frame->sample_rate);// 获取延迟样本数
    out_frame->nb_samples = av_rescale_rnd(
        delay + in_frame->nb_samples,
        out_samplerate_, in_frame->sample_rate,
        AV_ROUND_UP);// 计算输出样本数
    out_frame->pts = out_frame->pkt_dts = in_frame->pts;// 保持时间戳

    // 分配输出缓冲
    if (av_frame_get_buffer(out_frame.get(), 0) != 0)
    {
        return -1;
    }

    // 执行重采样
    int len = swr_convert(
        swr_context_,
        (uint8_t **)out_frame->data, out_frame->nb_samples,
        (const uint8_t **)in_frame->data, in_frame->nb_samples);// 重采样输入数据到输出缓冲
    if (len < 0)
    {
        out_frame.reset();
        return -1;
    }

    // 更新实际样本数
    out_frame->nb_samples = len;
    return len;
}

/**
 * @brief 初始化并打开重采样上下文
 * @param in_samplerate 源采样率（Hz）
 * @param in_channels 源通道数
 * @param in_format 源采样格式
 * @param out_samplerate 目标采样率（Hz）
 * @param out_channels 目标通道数
 * @param out_format 目标采样格式
 * @return true 初始化成功，false 失败
 */
bool AudioResampler::Open(
    int in_samplerate, int in_channels, AVSampleFormat in_format,
    int out_samplerate, int out_channels, AVSampleFormat out_format)
{
    if (swr_context_)
    {
        return false; // 已初始化
    }

    // 获取通道布局
    int64_t in_chan_layout = av_get_default_channel_layout(in_channels);
    int64_t out_chan_layout = av_get_default_channel_layout(out_channels);

    // 分配重采样上下文
    swr_context_ = swr_alloc();
    if (!swr_context_)
    {
        return false;
    }

    // 设置输入参数
    av_opt_set_int(swr_context_, "in_channel_layout", in_chan_layout, 0);// 设置输入通道布局
    av_opt_set_int(swr_context_, "in_sample_rate", in_samplerate, 0);// 设置输入采样率
    av_opt_set_int(swr_context_, "in_sample_fmt", in_format, 0);// 设置输入采样格式

    // 设置输出参数
    av_opt_set_int(swr_context_, "out_channel_layout", out_chan_layout, 0);// 设置输出通道布局
    av_opt_set_int(swr_context_, "out_sample_rate", out_samplerate, 0);// 设置输出采样率
    av_opt_set_int(swr_context_, "out_sample_fmt", out_format, 0);// 设置输出采样格式

    // 初始化重采样器
    if (swr_init(swr_context_) < 0)
    {
        swr_context_ = nullptr;
        return false;
    }

    // 保存格式信息
    in_samplerate_ = in_samplerate;
    in_channels_ = in_channels;
    in_bits_per_sample_ = av_get_bytes_per_sample(in_format);
    in_format_ = in_format;

    out_samplerate_ = out_samplerate;
    out_channels_ = out_channels;
    out_bits_per_sample_ = av_get_bytes_per_sample(out_format);
    out_format_ = out_format;

    return true;
}
