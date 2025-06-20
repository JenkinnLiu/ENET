/**
 * @file AudioCapture.cpp
 * @brief 实现 AudioCapture 类，封装 WASAPI 捕获与环形缓冲逻辑，提供音频初始化、读取和关闭功能
 * @date 2025-06-18
 */

#include "AudioCapture.h"
#include "AudioBuffer.h"
#include "WASAPICapture.h"
#include<QDebug>

/**
 * @brief 构造函数，创建 WASAPI 捕获实例和缓冲指针
 */
AudioCapture::AudioCapture()
    :capture_(nullptr)
    ,audio_buffer_(nullptr)
{
    capture_.reset(new WASAPICapture());
}

/**
 * @brief 析构函数，停止捕获并释放资源
 */
AudioCapture::~AudioCapture()
{

}

/**
 * @brief 初始化捕获流程，设置 PCM 缓冲区大小并启动捕获
 * @param size 环形缓冲区容量（字节）
 * @return true 初始化并启动成功，false 失败
 */
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
        WAVEFORMATEX* audoFmt = capture_->getAudioFormat();// 获取音频格式
        channels_ = audoFmt->nChannels;// 获取声道数
        samplerate_ = audoFmt->nSamplesPerSec; // 获取采样率
        bits_per_sample_ = audoFmt->wBitsPerSample; // 获取采样位数
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

/**
 * @brief 关闭捕获流程，停止采集并重置初始化状态
 */
void AudioCapture::Close()
{
    if(is_initailed_)
    {
        StopCapture();
        is_initailed_ = false;
    }
}

/**
 * @brief 获取当前缓冲区可读取的样本帧数
 * @return 可读取的 PCM 样本帧数
 */
int AudioCapture::GetSamples()
{
    //从缓冲区获取当前有多少音频数据
    return audio_buffer_->size() * 8 / bits_per_sample_ / channels_;
}

/**
 * @brief 从缓冲区读取指定数量的 PCM 样本帧
 * @param data 输出数据缓冲区，需提前分配足够空间
 * @param samples 读取的帧数
 * @return 实际读取的帧数，0 表示无数据或参数错误
 */
int AudioCapture::Read(uint8_t *data, uint32_t samples)
{
    //从缓冲区读数据
    if(samples > this->GetSamples()) //说明数不足
    {
        return 0;
    }
    // 计算需要读取的字节数,等于 samples * bits_per_sample / 8 * channels
    audio_buffer_->read((char*)data,samples * bits_per_sample_ / 8 * channels_);
    return samples;
}

/**
 * @brief 启动捕获器，将回调中的 PCM 数据写入环形缓冲
 * @return 0 成功，负值表示启动失败
 */
int AudioCapture::StartCapture()
{
    //开始捕获， 设置回调函数
    capture_->setCallback([this](const WAVEFORMATEX *mixFormat, uint8_t *data, uint32_t samples){
        channels_ = mixFormat->nChannels;// 获取声道数
        samplerate_ = mixFormat->nSamplesPerSec;// 获取采样率
        bits_per_sample_ = mixFormat->wBitsPerSample;// 获取采样位数
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

/**
 * @brief 停止捕获器并停止数据写入
 * @return 0 成功
 */
int AudioCapture::StopCapture()
{
    capture_->stop();
    is_stared_ = false;
    return 0;
}
