/**
 * @file AudioEncoder.h
 * @brief 定义 AudioEncoder 类，实现对 PCM 音频数据的重采样后编码为 AAC 格式的数据包
 * @date 2025-06-19
 */
#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include "AV_Common.h"
class AudioResampler;

/**
 * @class AudioEncoder
 * @brief 音频编码器，继承自 EncodBase，通过 AudioResampler 进行重采样，并使用 FFmpeg AAC 编码器生成 AVPacketPtr
 */
class AudioEncoder : public EncodBase
{
public:
    /**
     * @brief 构造函数，初始化内部重采样器指针为空
     */
    AudioEncoder();
    AudioEncoder(const AudioEncoder&) = delete;          ///< 禁用拷贝构造
    AudioEncoder& operator=(const AudioEncoder&) = delete;///< 禁用拷贝赋值
    /**
     * @brief 析构函数，调用 Close 释放资源
     */
    ~AudioEncoder();

public:
    /**
     * @brief 打开并初始化 AAC 编码器和重采样器
     * @param config 包含音频采样率、通道数、格式和码率等参数的 AVConfig 结构
     * @return true 打开成功，false 打开失败
     */
    virtual bool Open(AVConfig& config) override;
    /**
     * @brief 关闭编码器并释放重采样器资源
     */
    virtual void Close() override;
    /**
     * @brief 查询编码器每帧样本数量
     * @return 编码器内部帧大小 frame_size
     */
    uint32_t GetFrameSamples();
    /**
     * @brief 对输入的 PCM 缓冲进行重采样并编码
     * @param pcm 指向 PCM 原始数据的指针
     * @param samples PCM 数据的样本帧数
     * @return 包含 AAC 数据的 AVPacketPtr，失败时返回 nullptr
     */
    AVPacketPtr Encode(const uint8_t *pcm, int samples);

private:
    int64_t pts_ = 0;                                    ///< 当前音频帧的时间戳（基于样本点计数）
    std::unique_ptr<AudioResampler> audio_resampler_;    ///< 重采样器，用于将 PCM 转换成编码器支持的格式
};

#endif // AUDIO_ENCODER_H
