/**
 * @file Audio_Resampler.h
 * @brief 基于 FFmpeg SwResample 库的音频重采样器，用于不同采样率、通道数和采样格式之间的转换
 * @date 2025-06-19
 */
#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H

#include "AV_Common.h"
struct SwrContext;

/**
 * @class AudioResampler
 * @brief 音频重采样类，封装 SwrContext，实现音频帧的格式和采样率转换
 */
class AudioResampler
{
public:
    /**
     * @brief 构造函数，初始化内部重采样上下文指针
     */
    AudioResampler();

    /**
     * @brief 禁用拷贝构造，防止多实例冲突
     */
    AudioResampler(const AudioResampler&) = delete;

    /**
     * @brief 禁用赋值运算符
     */
    AudioResampler& operator=(const AudioResampler&) = delete;

    /**
     * @brief 析构函数，关闭并释放重采样上下文
     */
    ~AudioResampler();

public:
    /**
     * @brief 关闭重采样器，释放 SwrContext 资源
     */
    void Close();

    /**
     * @brief 执行重采样，将输入音频帧转换为目标格式
     * @param in_frame 输入音频帧智能指针，包含源格式数据
     * @param out_frame 输出音频帧智能指针（引用），返回重采样后的数据
     * @return 返回转换后的样本数，负值表示失败
     */
    int Convert(AVFramePtr in_frame, AVFramePtr& out_frame);

    /**
     * @brief 打开并配置重采样器上下文
     * @param in_samplerate 输入采样率（Hz）
     * @param in_channels 输入通道数
     * @param in_format 输入采样格式
     * @param out_samplerate 输出采样率（Hz）
     * @param out_channels 输出通道数
     * @param out_format 输出采样格式
     * @return true 初始化并打开成功，false 失败
     */
    bool Open(int in_samplerate, int in_channels, AVSampleFormat in_format,
              int out_samplerate, int out_channels, AVSampleFormat out_format);

private:
    SwrContext* swr_context_;        ///< 重采样上下文指针
    int in_samplerate_ = 0;          ///< 输入采样率
    int in_channels_ = 0;            ///< 输入通道数
    int in_bits_per_sample_ = 0;     ///< 输入位深（字节数）
    AVSampleFormat in_format_ = AV_SAMPLE_FMT_NONE;   ///< 输入格式

    int out_samplerate_ = 0;         ///< 输出采样率
    int out_channels_ = 0;           ///< 输出通道数
    int out_bits_per_sample_ = 0;    ///< 输出位深（字节数）
    AVSampleFormat out_format_ = AV_SAMPLE_FMT_NONE;  ///< 输出格式
};

#endif // AUDIO_RESAMPLER_H
