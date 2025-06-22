/**
 * @file AudioRender.cpp
 * @brief 音频渲染器实现
 *
 * 实现 AudioRender 类，包括音频设备初始化、可用缓冲查询
 * 以及 PCM 数据写入播放逻辑。
 */

#include "AudioRender.h"

/**
 * @brief 构造函数
 *
 * 设置默认音频格式（PCM、LittleEndian、SignedInt）。
 */
AudioRender::AudioRender()
{
    audioFmt_.setCodec("audio/pcm");// 编解码器
    audioFmt_.setByteOrder(QAudioFormat::LittleEndian);// 字节序
    audioFmt_.setSampleType(QAudioFormat::SignedInt);// 采样类型
}

/**
 * @brief 析构函数
 *
 * 释放音频输出对象和相关资源。
 */
AudioRender::~AudioRender()
{

}

/**
 * @brief 查询当前可用缓冲区大小
 * @return 剩余可写字节数，若未初始化返回 -1
 */
int AudioRender::AvailableBytes()
{
    //获取pcm大小
    if(!audioOut_)
    {
        return -1;
    }
    return audioOut_->bytesFree() - audioOut_->periodSize(); //剩余空间 - 每次周期需要填充的字节数
}

/**
 * @brief 初始化音频输出设备并开始播放
 * @param nChannels 声道数
 * @param SampleRate 采样率
 * @param nSampleSize 位宽
 * @return 成功返回 true，否则 false
 */
bool AudioRender::InitAudio(int nChannels, int SampleRate, int nSampleSize)
{
    //初始化音频输出
    if(audioOut_ || is_initail_ || device_)
    {
        return true;
    }

    nSampleSize_ = nSampleSize;

    //设置格式
    audioFmt_.setChannelCount(nChannels);
    audioFmt_.setSampleRate(SampleRate);
    audioFmt_.setSampleSize(nSampleSize);

    //创建输出
    audioOut_ = new QAudioOutput(audioFmt_);
    //设置缓冲区大小
    audioOut_->setBufferSize(409600);
    //设置音量
    audioOut_->setVolume(volume_);
    //创建接入设备
    device_ = audioOut_->start();
    is_initail_ = true;
    return true;
}

/**
 * @brief 写入 PCM 数据进行播放
 * @param frame PCM 数据帧，包含数据缓冲和样本信息
 */
void AudioRender::Write(AVFramePtr frame)
{
    //播放音频
    if(device_ && audioOut_ && nSampleSize_ != -1)
    {
        //获取frame数据
        QByteArray audioData(reinterpret_cast<char*>(frame->data[0]),(frame->nb_samples * frame->channels) * (nSampleSize_ / 8));
        //开始播放
        device_->write(audioData.data(),audioData.size());
        //释放这个frame
        av_frame_unref(frame.get());
    }
}
