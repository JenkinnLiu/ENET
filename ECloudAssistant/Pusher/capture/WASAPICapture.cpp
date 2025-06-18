/**
 * @file WASAPICapture.cpp
 * @brief 实现 WASAPICapture 类的音频捕获逻辑，包括初始化、启动、停止及数据回调
 * @date 2025-06-17
 */

#include "WASAPICapture.h"
#include <QDebug>

/**
 * @brief 构造函数，初始化 PCM 缓冲区大小并分配内存
 */
WASAPICapture::WASAPICapture()
{
    m_pcmBufSize = 4096;
    m_pcmBuf.reset(new uint8_t[m_pcmBufSize],std::default_delete<uint8_t[]>());
}

/**
 * @brief 析构函数，确保捕获停止并释放资源
 */
WASAPICapture::~WASAPICapture()
{

}

/**
 * @brief 初始化 WASAPI 组件，创建音频客户端并获取混合格式
 * @return 0 成功，非0 失败
 */
int WASAPICapture::init()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(m_initialized)
    {
        return 0;
    }

    //初始化这个COM库
    CoInitialize(NULL);

    HRESULT hr = S_OK;
    // 创建音频设备枚举器,用于获取默认音频设备
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL,IID_IMMDeviceEnumerator,(void**)m_enumerator.GetAddressOf());
    if(FAILED(hr))
    {
        qDebug() << "CoCreateInstance failed";
        return -1;
    }
    //获取默认音频设备
    hr = m_enumerator->GetDefaultAudioEndpoint(eRender,eMultimedia,m_device.GetAddressOf());
    if(FAILED(hr))
    {
        qDebug() << "GetDefaultAudioEndpoint failed";
        return -1;
    }

    //激活音频设备
    hr = m_device->Activate(IID_IAudioClient,CLSCTX_ALL,NULL,(void**)m_audioClient.GetAddressOf());
    if(FAILED(hr))
    {
        qDebug() << "Activate failed";
        return -1;
    }

    //获取音频格式
    hr = m_audioClient->GetMixFormat(&m_mixFormat);
    if(FAILED(hr))
    {
        qDebug() << "GetMixFormat failed";
        return -1;
    }

    //调整输出格式为16方便后续编码
    adjustFormatTo16Bits(m_mixFormat);

    //初始化音频客户端

    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,REFTIMES_PER_SEC,0,m_mixFormat,NULL);
    if(FAILED(hr))
    {
        qDebug() << "Initialize failed";
        return -1;
    }

    //获取缓冲区大小
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if(FAILED(hr))
    {
        qDebug() << "GetBufferSize failed";
        return -1;
    }

    //获取音频服务

    hr = m_audioClient->GetService(IID_IAudioCaptureClient,(void**)m_audioCaptureClient.GetAddressOf());
    if(FAILED(hr))
    {
        qDebug() << "GetService failed";
        return -1;
    }

    //计算这个buffer的时长, 用于后续计算音频包的持续时间
    // m_bufferFrameCount 是缓冲区的帧数
    // m_mixFormat->nSamplesPerSec 是采样率
    // m_HNSActualDuration 是缓冲区的实际持续时间, 等于REFRENCETIME(每秒的参考时间单位 * 帧数 / 采样率)
    m_hnsActualDuration = REFERENCE_TIME(REFTIMES_PER_SEC * m_bufferFrameCount / m_mixFormat->nSamplesPerSec);
    m_initialized = true;
    return 0;
}

/**
 * @brief 反初始化 WASAPI，释放 COM 库
 * @return 0 成功
 */
int WASAPICapture::exit()
{
    if(m_initialized)
    {
        m_initialized = false;
        CoUninitialize();
    }
    return 0;
}

/**
 * @brief 启动音频捕获线程，读取回环数据
 * @return 0 成功，非0 失败
 */
int WASAPICapture::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if(!m_initialized)
    {
        return -1;
    }

    if(m_isEnabeld)
    {
        return 0;
    }

    HRESULT hr = m_audioClient->Start(); //开始音频捕获
    if(FAILED(hr))
    {
        qDebug() << "m_audioClient->Start() failed";
        return -1;
    }

    m_isEnabeld = true;// 设置为启用状态
    //创建捕获线程
    m_threadPtr.reset(new std::thread([this](){
        while(this->m_isEnabeld)
        {
            if(this->capture() < 0)// 开始捕获，如果失败则退出
            {
                break;
            }
        }
    }));
    return 0;
}

/**
 * @brief 停止捕获线程并停止音频客户端
 * @return 0 成功，非0 失败
 */
int WASAPICapture::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_isEnabeld)
    {
        m_isEnabeld = false;// 设置为禁用状态,代表捕获线程将退出
        m_threadPtr->join();// 等待线程结束
        m_threadPtr.reset();// 释放线程资源
        m_threadPtr = nullptr;// 清空线程指针

        HRESULT hr = m_audioClient->Stop();// 停止音频捕获
        if(FAILED(hr))
        {
            qDebug() << "m_audioClient->Stop() failed";
            return -1;
        }
    }
    return 0;
}

/**
 * @brief 设置捕获完成后的数据回调函数
 * @param callback 用户定义的回调函数
 */
void WASAPICapture::setCallback(PacketCallback callback)
{
    m_callback = callback;
}

/**
 * @brief 将获取的 WAV 格式转换为 16 位 PCM
 * @param pwfx 指向 WAVEFORMATEX 结构的指针
 * @return 0 成功，非0 失败
 */
int WASAPICapture::adjustFormatTo16Bits(WAVEFORMATEX *pwfx)
{
    //设配16位
    if(pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)// 如果是 IEEE 浮点格式
    {
        pwfx->wFormatTag = WAVE_FORMAT_PCM; // 转换为 PCM 格式
    }
    else if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) // 如果是扩展格式
    {
        PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);// 强制转换为扩展格式指针
        if(IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,pEx->SubFormat))// 如果子格式是 IEEE 浮点
        {
            pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;// 子格式转换为 PCM 子格式
            pEx->Samples.wValidBitsPerSample = 16;// 设置有效位数为 16 位
        }
    }
    else
    {
        return -1;
    }
    pwfx->wBitsPerSample = 16;// 设置位深为 16 位
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;// 计算块对齐大小
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;// 计算平均字节率
    return 0;
}

/**
 * @brief 捕获循环函数，从音频捕获客户端读取 PCM 数据并触发回调
 * @return 0 成功或无数据，负值表示失败
 */
int WASAPICapture::capture()
{
    HRESULT hr = S_OK;// 初始化 HRESULT
    uint32_t packetLenght = 0;// 包大小
    uint32_t numFrameAvailabel = 0;// 可用帧数
    BYTE* pData;// 指向音频数据的指针
    DWORD flags;// 缓冲区标志

    //获取下一个包大小
    hr = m_audioCaptureClient->GetNextPacketSize(&packetLenght);// 获取下一个包的大小
    if(FAILED(hr))
    {
        qDebug() << "GetNextPacketSize failed";
        return -1;
    }

    if(packetLenght == 0) //没有数据
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return 0;
    }
    //有数据

    while (packetLenght > 0) {
        hr = m_audioCaptureClient->GetBuffer(&pData,&numFrameAvailabel,&flags,NULL,NULL);// 获取音频数据缓冲区
        if(FAILED(hr))
        {
            qDebug() << "m_audioCaptureClient->GetBuffer failed";
            return -1;
        }

        if(m_pcmBufSize < numFrameAvailabel * m_mixFormat->nBlockAlign) //缓冲区大小不足，需要扩容
        {
            m_pcmBufSize = numFrameAvailabel * m_mixFormat->nBlockAlign;// 计算新的缓冲区大小
            m_pcmBuf.reset(new uint8_t[m_pcmBufSize],std::default_delete<uint8_t[]>());
        }

        if(flags & AUDCLNT_BUFFERFLAGS_SILENT) //当前是没有声音 我们就去传空
        {
            qDebug() << "AUDCLNT_BUFFERFLAGS_SILENT";
            memset(m_pcmBuf.get(),0,m_pcmBufSize);
        }

        else//当前有声音
        {
            memcpy(m_pcmBuf.get(),pData,numFrameAvailabel * m_mixFormat->nBlockAlign);// 将捕获的数据复制到 PCM 缓冲区
        }

        if(m_callback)
        {
            m_callback(m_mixFormat,pData,numFrameAvailabel);// 调用用户定义的回调函数处理捕获的数据
        }

        //释放这个缓存
        hr = m_audioCaptureClient->ReleaseBuffer(numFrameAvailabel);
        if(FAILED(hr))
        {
            qDebug() << "ReleaseBuffer failed";
            return -1;
        }

        //获取下一个包的大小；
        hr = m_audioCaptureClient->GetNextPacketSize(&packetLenght);
        if(FAILED(hr))
        {
            qDebug() << "GetNextPacketSize failed";
            return -1;
        }
    }
    return 0;
}
