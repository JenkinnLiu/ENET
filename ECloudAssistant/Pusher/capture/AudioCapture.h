/**
 * @file AudioCapture.h
 * @brief 音频捕获管理类，封装 WASAPI 回环捕获与环形缓冲，实现系统音频数据的初始化、启动、读取与关闭功能
 * @date 2025-06-18
 */
#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <thread>
#include <cstdint>
#include <memory>

class AudioBuffer;
class WASAPICapture;

/**
 * @class AudioCapture
 * @brief 管理音频捕获流程，提供初始化、启动、停止以及数据读取接口
 */
class AudioCapture
{
public:
    /**
     * @brief 构造函数，创建底层 WASAPI 捕获实例
     */
    AudioCapture();

    /**
     * @brief 析构函数，确保捕获停止并释放资源
     */
    ~AudioCapture();

public:
    /**
     * @brief 初始化捕获器与环形缓冲
     * @param size 缓冲区大小（字节），默认为 20480
     * @return true 初始化成功或已初始化，false 初始化失败
     */
    bool Init(uint32_t size = 20480);

    /**
     * @brief 关闭捕获器并释放资源
     */
    void Close();

    /**
     * @brief 查询缓冲区中可读取的样本帧数
     * @return 可读取的样本帧数
     */
    int GetSamples();

    /**
     * @brief 从缓冲区读取指定数量的 PCM 样本数据
     * @param data 输出数据缓冲
     * @param samples 读取的样本帧数
     * @return 实际读取的样本帧数，0 表示无数据
     */
    int Read(uint8_t* data, uint32_t samples);

    /**
     * @brief 检查捕获是否已启动
     * @return true 已启动，false 未启动
     */
    inline bool CaptureStarted() const { return is_stared_; }

    /**
     * @brief 获取音频通道数
     * @return 通道数
     */
    inline uint32_t GetChannels() const { return channels_; }

    /**
     * @brief 获取采样率（Hz）
     * @return 采样率
     */
    inline uint32_t GetSamplerate() const { return samplerate_; }

    /**
     * @brief 获取每样本位深（位）
     * @return 位深
     */
    inline uint32_t GetBitsPerSample() const { return bits_per_sample_; }

private:
    /**
     * @brief 启动内部捕获并将数据写入环形缓冲
     * @return 0 成功，负值失败
     */
    int StartCapture();

    /**
     * @brief 停止内部捕获
     * @return 0 成功
     */
    int StopCapture();

private:
    bool is_initailed_ = false;                 ///< 是否已初始化
    bool is_stared_ = false;                    ///< 捕获启动状态
    uint32_t channels_ = 2;                     ///< 音频通道数
    uint32_t samplerate_ = 48000;               ///< 采样率
    uint32_t bits_per_sample_ = 16;             ///< 每样本位深
    std::unique_ptr<WASAPICapture> capture_;    ///< WASAPI 捕获实例
    std::unique_ptr<AudioBuffer> audio_buffer_;///< PCM 数据环形缓冲区
};

#endif // AUDIOCAPTURE_H
