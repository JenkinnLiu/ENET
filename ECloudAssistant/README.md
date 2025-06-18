# 客户端推流模块
## WASAPICapture.cpp 实现 WASAPICapture 类的音频捕获逻辑，包括初始化、启动、停止及数据回调

## 以下分步骤详细说明 WASAPICapture 类的工作流程：

1. 初始化（init）  
    • 调用 CoInitialize 初始化 COM 环境  
    • 通过 CoCreateInstance(CLSID_MMDeviceEnumerator) 创建 设备枚举器IMMDeviceEnumerator  
    • 调用 GetDefaultAudioEndpoint(eRender, eMultimedia) 获得默认回环（Loopback）音频设备  
    • 激活设备的 IAudioClient 接口（Activate）  
    • 通过 IAudioClient::GetMixFormat 获取系统混合格式（WAVEFORMATEX）  
    • 调用 adjustFormatTo16Bits 将浮点或扩展格式转换为 16 位 PCM（更新 wBitsPerSample、nBlockAlign、nAvgBytesPerSec）  
    • 使用 IAudioClient::Initialize(共享模式, 回环标志, 缓冲参考时间, …) 初始化客户端  
    • 获取缓冲区帧数（GetBufferSize）并计算参考时间长度（m_hnsActualDuration）  
    • 通过 IAudioClient::GetService(IID_IAudioCaptureClient) 获取 IAudioCaptureClient  

2. 设置回调（setCallback）  
    • 将用户提供的 PacketCallback 函数保存到 m_callback，后续捕获到音频后会调用  

3. 启动捕获（start）  
    • 调用 IAudioClient::Start() 开始音频流  
    • 在后台线程中循环调用 capture()：  
      – GetNextPacketSize 获取下一批可用帧数  
      – 若为 0，则短暂 sleep 后重试  
      – 使用 IAudioCaptureClient::GetBuffer 获取指向 PCM 数据的指针及帧数、状态标志  
      – 根据标志判断是否为静音，如是则填充 0，否则 memcpy 到自有缓冲区  
      – 若 PacketCallback 已设置，则以 (格式指针, 数据指针, 帧数) 调用回调  
      – ReleaseBuffer 释放底层缓冲，继续读取直到无数据  

4. 停止捕获（stop）  
    • 将 m_isEnabled 置 false，后台线程退出循环并 join  
    • 调用 IAudioClient::Stop() 停止音频流  

5. 反初始化（exit）  
    • 若已初始化则调用 CoUninitialize 释放 COM  

6. 辅助方法  
    • adjustFormatTo16Bits：将 IEEE_FLOAT 或 EXTENSIBLE→PCM、设置位深为 16、更新对齐与字节率  
    • getAudioFormat：返回当前使用的 WAVEFORMATEX*，供外部查询采样率、通道数等信息  

整体上，WASAPICapture 通过 WASAPI 的 Loopback 模式抓取“系统播放”音频，转换为 16 位 PCM，并通过用户回调输出，适合用作录屏/推流等场景的音频源。

## 简要描述：客户端使用 WASAPI 捕获音频的典型流程如下：

1. 初始化 COM 环境  
   • CoInitialize(NULL)  

2. 获取默认回环（Loopback）音频设备  
   • CoCreateInstance(CLSID_MMDeviceEnumerator) → IMMDeviceEnumerator  
   • GetDefaultAudioEndpoint(eRender, eMultimedia) → IMMDevice  

3. 激活并配置 IAudioClient  
   • device->Activate(IID_IAudioClient) → IAudioClient  
   • GetMixFormat(&mixFormat) → 获取系统混合格式  
   • 将浮点/扩展格式转换为 16 位 PCM（调整 wBitsPerSample、nBlockAlign、nAvgBytesPerSec）  
   • Initialize(共享模式 | 回环标志, …, mixFormat) → 初始化客户端  
   • GetBufferSize(&bufferFrameCount) → 获取缓冲区帧数  
   • GetService(IID_IAudioCaptureClient) → IAudioCaptureClient  

4. 启动捕获  
   • audioClient->Start()  
   • 在后台线程中循环：  
     – GetNextPacketSize(&packetFrames)  
     – GetBuffer(&pData, &frameCount, &flags)  
     – 如果静音则填零，否则复制到内部 PCM 缓冲  
     – 通过回调函数 (mixFormat, data, frameCount) 输出数据  
     – ReleaseBuffer(frameCount)  

5. 停止与清理  
   • 标记线程退出并 join  
   • audioClient->Stop()  
   • CoUninitialize()  

这一流程利用 WASAPI 的共享回环模式，将系统播放的音频捕获为 16 位 PCM 数据，并通过用户回调实时输出。

## 什么是loopback共享回环模式？

Loopback 模式就是在“共享”（Shared）模式下，不是从麦克风或输入设备采集音频，而是直接“回环”捕获系统播放（Render）的混合输出流。这样你能拿到当前在扬声器/耳机里实际听到的所有声音。

在代码里，loopback 模式通过给 IAudioClient::Initialize 传入 AUDCLNT_STREAMFLAGS_LOOPBACK 标志来开启，具体位置在 WASAPICapture::init() 中：

```cpp
hr = m_audioClient->Initialize(
         AUDCLNT_SHAREMODE_SHARED,         // 共享模式
         AUDCLNT_STREAMFLAGS_LOOPBACK,     // ← 这里打开回环捕获
         REFTIMES_PER_SEC, 
         0,
         m_mixFormat,
         NULL);
```

– AUDCLNT_SHAREMODE_SHARED：表示与系统音频引擎共享同一渲染流。  
– AUDCLNT_STREAMFLAGS_LOOPBACK：表示要捕获的是渲染（输出）端的音频，而不是输入。