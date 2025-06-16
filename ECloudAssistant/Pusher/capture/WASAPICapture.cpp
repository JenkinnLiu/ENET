#include "WASAPICapture.h"
#include <QDebug>

WASAPICapture::WASAPICapture()
{
    m_pcmBufSize = 4096;
    m_pcmBuf.reset(new uint8_t[m_pcmBufSize],std::default_delete<uint8_t[]>());
}

WASAPICapture::~WASAPICapture()
{

}

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
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL,IID_IMMDeviceEnumerator,(void**)m_enumerator.GetAddressOf());
    if(FAILED(hr))
    {
        qDebug() << "CoCreateInstance failed";
        return -1;
    }

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

    //计算这个buffer的时长
    m_hnsActualDuration = REFERENCE_TIME(REFTIMES_PER_SEC * m_bufferFrameCount / m_mixFormat->nSamplesPerSec);
    m_initialized = true;
    return 0;
}

int WASAPICapture::exit()
{
    if(m_initialized)
    {
        m_initialized = false;
        CoUninitialize();
    }
    return 0;
}

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

    HRESULT hr = m_audioClient->Start();
    if(FAILED(hr))
    {
        qDebug() << "m_audioClient->Start() failed";
        return -1;
    }

    m_isEnabeld = true;
    m_threadPtr.reset(new std::thread([this](){
        while(this->m_isEnabeld)
        {
            if(this->capture() < 0)
            {
                break;
            }
        }
    }));
    return 0;
}

int WASAPICapture::stop()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_isEnabeld)
    {
        m_isEnabeld = false;
        m_threadPtr->join();
        m_threadPtr.reset();
        m_threadPtr = nullptr;

        HRESULT hr = m_audioClient->Stop();
        if(FAILED(hr))
        {
            qDebug() << "m_audioClient->Stop() failed";
            return -1;
        }
    }
    return 0;
}

void WASAPICapture::setCallback(PacketCallback callback)
{
    m_callback = callback;
}

int WASAPICapture::adjustFormatTo16Bits(WAVEFORMATEX *pwfx)
{
    //设配16位
    if(pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        pwfx->wFormatTag = WAVE_FORMAT_PCM; /////////////////////////// ==
    }
    else if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
        if(IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,pEx->SubFormat))
        {
            pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            pEx->Samples.wValidBitsPerSample = 16;
        }
    }
    else
    {
        return -1;
    }
    pwfx->wBitsPerSample = 16;
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
    return 0;
}

int WASAPICapture::capture()
{
    HRESULT hr = S_OK;
    uint32_t packetLenght = 0;
    uint32_t numFrameAvailabel = 0;
    BYTE* pData;
    DWORD flags;

    //获取下一个包大小
    hr = m_audioCaptureClient->GetNextPacketSize(&packetLenght);
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
        hr = m_audioCaptureClient->GetBuffer(&pData,&numFrameAvailabel,&flags,NULL,NULL);
        if(FAILED(hr))
        {
            qDebug() << "m_audioCaptureClient->GetBuffer failed";
            return -1;
        }

        if(m_pcmBufSize < numFrameAvailabel * m_mixFormat->nBlockAlign) //缓冲区大小不足，需要扩容
        {
            m_pcmBufSize = numFrameAvailabel * m_mixFormat->nBlockAlign;
            m_pcmBuf.reset(new uint8_t[m_pcmBufSize],std::default_delete<uint8_t[]>());
        }

        if(flags & AUDCLNT_BUFFERFLAGS_SILENT) //当前是没有声音 我们就去传空
        {
            qDebug() << "AUDCLNT_BUFFERFLAGS_SILENT";
            memset(m_pcmBuf.get(),0,m_pcmBufSize);
        }

        else
        {
            memcpy(m_pcmBuf.get(),pData,numFrameAvailabel * m_mixFormat->nBlockAlign);
        }

        if(m_callback)
        {
            m_callback(m_mixFormat,pData,numFrameAvailabel);
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
