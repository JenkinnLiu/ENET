/**
 * @file AudioRender.h
 * @brief 音频渲染器类定义
 *
 * 封装基于 Qt QAudioOutput 的 PCM 音频播放功能，
 * 提供初始化音频设备、获取可用缓冲区大小和写入 PCM 数据。
 */

#ifndef AUDIO_RENDER_H
#define AUDIO_RENDER_H
#include <QAudioOutput>
#include "AV_Common.h"

/**
 * @class AudioRender
 * @brief 音频输出渲染器
 *
 * 管理音频格式设置、缓冲区创建及 PCM 数据写入，
 * 通过 QAudioOutput 在系统默认音频设备上播放声音。
 */
class AudioRender
{
public:
    /**
     * @brief 构造函数，初始化音频格式参数
     */
    AudioRender();

    /**
     * @brief 析构函数，释放音频输出资源
     */
    ~AudioRender();

    /**
     * @brief 检查渲染器是否已初始化
     * @return true 表示已初始化音频输出，false 表示未初始化
     */
    inline bool IsInit(){return is_initail_;}

    /**
     * @brief 查询当前可写入的 PCM 缓冲区大小
     * @return 可用字节数，<0 表示未初始化
     */
    int AvailableBytes();

    /**
     * @brief 初始化音频输出设备
     * @param nChannels 声道数（如 1 单声道、2 立体声）
     * @param SampleRate 采样率（如 44100）
     * @param nSampleSize 采样位宽（如 16）
     * @return 成功返回 true，失败返回 false
     */
    bool InitAudio(int nChannels,int SampleRate,int nSampleSize);

    /**
     * @brief 播放 PCM 音频帧数据
     * @param frame PCM 数据帧，包含 nb_samples 和 channels 信息
     */
    void Write(AVFramePtr frame);

private:
    bool is_initail_ = false;        ///< 标记渲染器是否已初始化
    int  nSampleSize_ = -1;          ///< 每个样本的位宽
    int  volume_ = 50;               ///< 音量（0-100）
    QAudioFormat  audioFmt_;         ///< 音频格式配置
    QIODevice*    device_ = nullptr; ///< QAudioOutput 的输出设备指针
    QAudioOutput* audioOut_ = nullptr;///< Qt 音频输出对象
};

#endif // AUDIO_RENDER_H
