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

class WASAPICapture
{
public:
    typedef std::function<void(const WAVEFORMATEX *mixFormat, uint8_t *data, uint32_t samples)> PacketCallback;
    WASAPICapture();
    WASAPICapture(const WASAPICapture&) = delete;
    WASAPICapture& operator=(const WASAPICapture&) = delete;
    ~WASAPICapture();
    int init();
    int exit();
    int start();
    int stop();
    void setCallback(PacketCallback callback);
    WAVEFORMATEX *getAudioFormat() const
    {
        return m_mixFormat;
    }
private:
    bool m_initialized = false;
    bool m_isEnabeld = false;
    int adjustFormatTo16Bits(WAVEFORMATEX *pwfx);
    int capture();
    const int REFTIMES_PER_SEC = 10000000;
    const int REFTIMES_PER_MILLISEC = 10000;
    const IID IID_IAudioClient = __uuidof(IAudioClient);
    const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);

    std::mutex m_mutex;
    uint32_t m_pcmBufSize;
    uint32_t m_bufferFrameCount;
    PacketCallback m_callback;
    WAVEFORMATEX *m_mixFormat = NULL;
    std::shared_ptr<uint8_t> m_pcmBuf; //捕获之后pcm缓存的这个pcmBuf中
    REFERENCE_TIME m_hnsActualDuration;
    std::shared_ptr<std::thread> m_threadPtr;
    Microsoft::WRL::ComPtr<IMMDevice> m_device;
    Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_enumerator;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_audioCaptureClient;
};
#endif
