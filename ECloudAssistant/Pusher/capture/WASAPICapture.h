/**
 * @file WASAPICapture.h
 * @brief 基于 WASAPI 实现的音频捕获类，支持共享模式下的回环捕获并通过回调输出 PCM 数据
 * @date 2025-06-17
 */
#ifndef WASAPI_CAPTURE_H 
#define WASAPI_CAPTURE_H

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <wrl.h>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <mutex>
#include <memory>
#include <thread>

/**
 * @class WASAPICapture
 * @brief 封装 WASAPI 音频捕获流程，提供初始化、启动、停止及数据回调接口
 */
class WASAPICapture
{
public:
    /**
     * @typedef PacketCallback
     * @brief 捕获到音频数据后的回调函数类型
     * @param mixFormat 捕获的混合格式描述
     * @param data PCM 原始数据指针
     * @param samples 样本帧数
     */
    typedef std::function<void(const WAVEFORMATEX *mixFormat,
                               uint8_t *data,
                               uint32_t samples)> PacketCallback;

    /**
     * @brief 构造函数，分配默认缓冲区
     */
    WASAPICapture();

    /**
     * @brief 禁用拷贝构造，防止多实例冲突
     */
    WASAPICapture(const WASAPICapture&) = delete;

    /**
     * @brief 禁用拷贝赋值运算符
     */
    WASAPICapture& operator=(const WASAPICapture&) = delete;

    /**
     * @brief 析构函数，确保停止捕获并释放资源
     */
    ~WASAPICapture();

    /**
     * @brief 初始化 COM 及音频客户端，获取混合格式并准备捕获
     * @return 0 成功，非0 为失败
     */
    int init();

    /**
     * @brief 反初始化操作，释放 COM 资源
     * @return 0 成功
     */
    int exit();

    /**
     * @brief 启动回环捕获线程
     * @return 0 成功，非0 为失败
     */
    int start();

    /**
     * @brief 停止捕获并终止后台线程
     * @return 0 成功，非0 为失败
     */
    int stop();

    /**
     * @brief 设置捕获数据回调
     * @param callback 用户定义的数据处理函数
     */
    void setCallback(PacketCallback callback);

    /**
     * @brief 获取音频格式信息
     * @return 当前捕获使用的 WAVEFORMATEX 格式描述
     */
    WAVEFORMATEX *getAudioFormat() const
    {
        return m_mixFormat;
    }

private:
    bool m_initialized = false;                ///< 是否已完成 init
    bool m_isEnabeld = false;                 ///< 是否已启动捕获

    /**
     * @brief 将格式转换为 16 位 PCM
     * @param pwfx 要调整的格式指针
     * @return 0 成功，非0 失败
     */
    int adjustFormatTo16Bits(WAVEFORMATEX *pwfx);

    /**
     * @brief 捕获循环体，读取一帧数据并触发回调
     * @return 0 成功，负值失败或结束
     */
    int capture();

    const int REFTIMES_PER_SEC = 10000000;     ///< 每秒参考时间单位
    const int REFTIMES_PER_MILLISEC = 10000;   ///< 每毫秒参考时间单位
    const IID IID_IAudioClient = __uuidof(IAudioClient);// 音频客户端接口
    const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);// 音频捕获客户端接口
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);// 设备枚举器接口
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);// MM 设备枚举器类

    std::mutex m_mutex;                        ///< 保护初始化与状态切换
    uint32_t m_pcmBufSize;                     ///< PCM 缓冲区当前大小
    uint32_t m_bufferFrameCount;               ///< 音频缓冲区帧数
    PacketCallback m_callback;                 ///< 用户数据回调
    WAVEFORMATEX *m_mixFormat = nullptr;       ///< 获取的混合格式描述
    std::shared_ptr<uint8_t> m_pcmBuf;         ///< PCM 数据缓冲区
    REFERENCE_TIME m_hnsActualDuration;        ///< 缓冲区持续时间
    std::shared_ptr<std::thread> m_threadPtr;  ///< 捕获后台线程
    Microsoft::WRL::ComPtr<IMMDevice> m_device;               ///< 音频渲染设备
    Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;       ///< WASAPI 客户端
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_enumerator;  ///< 设备枚举器
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_audioCaptureClient; ///< 音频捕获客户端
};

#endif // WASAPI_CAPTURE_H
