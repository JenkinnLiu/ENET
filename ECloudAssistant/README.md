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

## 音频采集缓冲区AudioBuffer.h
环形缓冲区（Circular Buffer）是一种固定容量的 FIFO 队列，它用一段连续的内存和两个索引（读索引 readerIndex、写索引 writerIndex）来管理数据的读写。当写指针或读指针到达缓冲区末尾时，会“绕回”到缓冲区开头，从而循环利用内存，不必做整体移动。  

在 AudioBuffer.h 中，以下部分体现了环形缓冲区的原理：  

1. **读写索引**  
   - `_readerIndex`：当前可读数据开始位置  
   - `_writerIndex`：当前可写数据开始位置  
   ```cpp
   size_t _readerIndex = 0;
   size_t _writerIndex = 0;
   ```

2. **可读/可写字节计算**  
   - readableBytes() = 写索引 – 读索引  
   - writableBytes() = 缓冲大小 – 写索引  
   ```cpp
   uint32_t readableBytes() const { return _writerIndex - _readerIndex; }
   uint32_t writableBytes() const { return _buffer.size() - _writerIndex; }
   ```

3. **写入数据**  
   - 将外部数据拷贝到 `beginWrite()`（写入位置），然后推进 `_writerIndex`  
   ```cpp
   memcpy(beginWrite(), data, size);
   _writerIndex += size;
   ```

4. **读取数据**  
   - 从 `peek()`（读取位置）拷贝到用户缓冲，推进 `_readerIndex`  
   ```cpp
   memcpy(data, peek(), size);
   retrieve(size);
   ```

5. **索引绕回 & 数据搬移**  
   - `retrieve(len)` 在推进读索引后，如果已读数据到达写索引或读索引不为 0，就将剩余数据整体左移到缓冲区头部，重置读写索引  
   ```cpp
   if (_readerIndex > 0 && _writerIndex > 0) {
       _buffer.erase(_buffer.begin(), _buffer.begin() + _readerIndex);
       _buffer.resize(_bufferSize);
       _writerIndex -= _readerIndex;
       _readerIndex = 0;
   }
   ```
   这段逻辑等价于把“已读部分”腾出的空间循环利用，写指针也能绕回到空间前端。

通过以上机制，`AudioBuffer` 在固定大小内循环读写，不断复用已读区域，从而实现了环形缓冲区。

## 采集音频全流程
音频采集从最底层的 WASAPI 设备到上层读数据，大致可以分为以下几步：

1. 对象创建  
   • 构造 `AudioCapture` 时会内部创建一个 `WASAPICapture` 实例，准备后续捕获。  

2. 初始化  
   • 调用 `AudioCapture::Init(size)`  
     – `WASAPICapture::init()`：  
       1) CoInitialize COM  
       2) 枚举默认回环（Loopback）渲染设备  
       3) Activate 出 `IAudioClient`、获取混合格式 `WAVEFORMATEX`  
       4) 强制转换为 16 位 PCM  
       5) Initialize 为共享+Loopback 模式  
       6) GetBufferSize／GetService 拿到 `IAudioCaptureClient`  
     – 上层拿到采样率、通道数、位深  
     – 创建一个指定字节数的 `AudioBuffer` 环形缓冲  

3. 启动采集  
   • `AudioCapture::StartCapture()`：  
     – 给 `WASAPICapture` 注册一个回调，把 `Capture` 回来的 PCM 数据、位深、采样率等数据 `AudioBuffer.write()`  
     – 清空缓冲区，调用 `WASAPICapture::start()` 启动后台线程  

4. 底层循环捕获（WASAPICapture::capture）  
   循环里：  
   1) `GetNextPacketSize` 获得帧数  
   2) `GetBuffer`／flags 判断静音或正常数据  
   3) 拷贝到内部临时 PCM 缓冲  
   4) 调用上层回调（将数据写入环形缓冲）  
   5) `ReleaseBuffer`  
   6) 再次 `GetNextPacketSize` …  

5. 环形缓冲管理（AudioBuffer）  
   • `write()`：锁、检测可写空间、memcpy 数据、维护读写索引、在必要时把未读数据移动到缓冲头部  
   • `read()`：锁、检测可读量、memcpy 到用户缓冲、推进读索引并可能做一次整体移动  
   • 这样保证在固定大小内不断复用内存，不丢数据、不卡顿  

6. 用户读取  
   • 调用 `AudioCapture::GetSamples()` 查询可用帧数  
   • 调用 `AudioCapture::Read(buf, n)` 从环形缓冲中一次性读出指定帧数的 PCM 数据  

7. 停止与销毁  
   • `AudioCapture::Close()` → 调用 `WASAPICapture::stop()` 停止线程并 `IAudioClient::Stop()`  
   • `WASAPICapture::exit()` → CoUninitialize，释放 COM  
   • 析构时自动回收线程、缓冲等资源  

完整流程确保了：  
- 底层实时从系统回环音频流获取 PCM  
- 使用 Loopback 模式不干扰麦克风输入  
- 将数据写入线程安全的环形缓冲，方便上层随时按需拉取  
- 通过统一接口对外提供采样率/通道/位深信息和原始 PCM 数据。

## 用FFmpeg的gdigrab采集视频全流程
GDIScreenCapture 的完整工作流程可以分为以下几个阶段：

1. 对象创建  
   - 构造函数中调用 `avdevice_register_all()` 注册 FFmpeg 的设备支持。  
   - 分配一个 AVFrame 和一个 AVPacket 结构，用于后续存放原始数据和解码数据。

2. 初始化（Init）  
   - 如果已经初始化过则直接返回成功。  
   - 构造一个 `AVDictionary`，设置抓屏参数：  
     • “framerate”：目标帧率（默认 25 fps）  
     • “draw_mouse”：是否渲染鼠标指针  
     • “video_size”：捕获分辨率（如“2560x1440”）  
   - 调用 `av_find_input_format("gdigrab")` 获取 GDI 回环设备格式。  
   - `avformat_alloc_context()` 分配格式上下文；`avformat_open_input(..., "desktop", input_format_, &options)` 打开“desktop”源。  
   - `avformat_find_stream_info()` 读取流信息；遍历 `format_context_->streams[]` 找到视频流索引。  
   - 通过 `avcodec_find_decoder()` 查找对应解码器，并用 `avcodec_alloc_context3()` 分配上下文。  
   - 将视频流的 codecpar 参数拷贝到解码上下文，再调用 `avcodec_open2()` 打开解码器。  
   - 标记初始化完成并启动线程（调用 `start()`，内部会执行 `run()`）。

3. 后台线程循环（run）  
   - 只要 `is_initialzed_` 为真且 `stop_` 为假，就以设定的帧率（1000/framerate_ 毫秒）为间隔循环：  
     • 调用 `GetOneFrame()` 读取并解码一帧；  

4. 读取一帧并解码（GetOneFrame + Decode）  
   - `av_read_frame(format_context_, av_packet)` 从 FFmpeg 流中拉取一个包；  
   - 如果包属于视频流，则调用 `Decode(av_frame, av_packet)`：  
     1) `avcodec_send_packet()` 将包送入解码器  
     2) `avcodec_receive_frame()` 获取解码后的 AVFrame  
     3) 为 RGBA 数据分配缓冲区（`new uint8_t[frame_size]`）  
     4) 将 AVFrame 中的像素行（`av_frame->data[0]`）按行 memcpy 到 RGBA 缓冲  
     5) 更新 `width_`、`height_`、`frame_size_`，并加锁保存到 `rgba_frame_` 指针  
   - `av_packet_unref()` 释放包引用  

5. 外部获取帧（CaptureFrame）  
   - 在主线程调用时加锁访问：  
     • 如果未初始化或已停止，返回空。  
     • 确保输出容器 `rgba` 有足够容量，调用 `rgba.assign()` 将 `rgba_frame_` 数据拷贝出来。  
     • 返回当前 `width_`、`height_`。  

6. 停止与清理（Close & 析构）  
   - `Close()`：先调用 `StopCapture()` 停止线程——设置 `stop_ = true`，`quit()`、`wait()` 等待线程退出，并清空帧缓存；  
   - 释放解码器上下文（`avcodec_close`）、格式上下文（`avformat_close_input`）和其它指针；  
   - 析构函数再次调用 `Close()` 确保彻底释放资源。  

——  
通过上述流程，GDIScreenCapture 利用 FFmpeg 的“gdigrab”设备在后台循环拉取屏幕帧，解码为 RGBA 原始像素数据，并通过线程安全的方式让外部调用 `CaptureFrame()` 即时获取最新帧，从而实现高效的屏幕捕获。


## 中文能多一点吗？
下面以“gdigrab”设备为例，分步骤说明 FFmpeg 屏幕采集的完整流程，并尽量用中文术语替换核心 API 名称。

1. 注册采集设备  
   • 调用“注册所有设备”（avdevice_register_all），让 FFmpeg 识别“gdigrab”屏幕采集模块。

2. 构造参数字典（AVDictionary）  
   • 新建“字典上下文”，设置采集帧率、是否绘制鼠标、屏幕分辨率等：  
     – “帧率”=25  
     – “绘制鼠标”=1  
     – “视频大小”=“2560x1440”  

3. 查找并打开输入格式  
   • 调用“查找输入格式”（av_find_input_format）定位“gdigrab”设备格式  
   • 调用“打开输入流”（avformat_open_input），打开源“desktop”（主屏幕），并传入上面构造的参数字典  
   • 返回“格式上下文”结构，用于后续流读取

4. 读取并解析流信息  
   • 调用“读取流信息”（avformat_find_stream_info），扫描格式上下文中的所有媒体流，填充各流元数据

5. 定位视频流  
   • 遍历“格式上下文→流列表”，找到“视频流”（media_type=AVMEDIA_TYPE_VIDEO），记录其流索引

6. 查找视频解码器  
   • 根据视频流的编码 ID，调用“查找解码器”（avcodec_find_decoder），获得对应解码器

7. 分配并配置解码器上下文  
   • 调用“分配解码上下文”（avcodec_alloc_context3），创建解码器上下文  
   • 调用“拷贝流参数到解码上下文”（avcodec_parameters_to_context），将分辨率、像素格式等参数复制进去  
   • 将输出像素格式指定为 RGBA（AV_PIX_FMT_RGBA），方便后续直接使用真彩色数据  

8. 打开解码器  
   • 调用“打开解码器”（avcodec_open2），初始化解码器，使其准备好接收压缩包并输出原始帧

9. 启动后台采集线程  
   • 派生自 QThread 的类调用 start()，进入独立线程执行 run() 方法，开始循环采集

10. 循环读取压缩包  
    在 run() 中以“睡眠→读取”方式按25帧率进行循环：  
    • 调用“读取数据包”（av_read_frame），从格式上下文拿到一个 AVPacket  
    • 判断包属于视频流后，进入解码流程

11. 解码视频帧  
    • 调用“发送数据包到解码器”（avcodec_send_packet）  
    • 调用“接收解码帧”（avcodec_receive_frame），拿到一帧 AVFrame（YUV 或平台输出格式）  
    • 调用“释放数据包引用”（av_packet_unref）

12. 像素格式转换（如需）  
    如果解码输出非 RGBA，可用 sws_scale（图像格式转换）将 AVFrame 转为 RGBA。上例中已在解码上下文中直接指定了 RGBA。

13. 拷贝到应用缓冲  
    • 根据帧宽高和像素深度，计算所需字节数  
    • 分配或复用一段连续内存，将 AVFrame 按行 memcpy 到 RGBA 缓冲区  
    • 用互斥锁保护，将新缓冲指针和帧尺寸保存到成员变量

14. 对外提供抓取接口  
    • 用户调用“获取帧数据”（CaptureFrame）时，加锁复制缓冲区内容到外部容器，并返回当前宽度/高度

15. 停止与资源清理  
    • 调用 stop 标志停止循环，quit()/wait() 等待线程退出  
    • 调用“关闭解码器”（avcodec_close）、“关闭输入流”（avformat_close_input）、释放 AVFrame/AVPacket、销毁格式和解码上下文  
    • 最后可再次调用 CoUninitialize（如有 COM 初始化需求）

通过以上步骤，FFmpeg 的 gdigrab 模式即可将桌面画面按设定帧率不断抓取、解码为 RGBA 像素数据，并通过线程安全的方式提供给应用层使用。

## FFmpeg的gdigrab在解码之前是如何抓取屏幕的？代码里有体现吗
在你贴出的代码里，并没有任何直接调用 GDI（如 BitBlt）API 来截图的地方——这部分“屏幕抓取”逻辑都被 FFmpeg 的 gdigrab 设备封装在内部了。你在代码里体现它的关键在于：

1. 在 Init() 里选择并打开“gdigrab”设备：  
   ```cpp
   input_format_ = av_find_input_format("gdigrab");           // 找到 gdigrab 输入格式
   avformat_open_input(&format_context_, "desktop",              // 打开“desktop”源（主屏幕）
                       input_format_, &options);
   ```
   这两行就告诉 FFmpeg：后续所有的帧读取都从 Windows 屏幕捕获，而不是文件或网络。

2. 在 GetOneFrame()（或 av_read_frame）中触发抓屏：  
   ```cpp
   av_read_frame(format_context_, av_packet_.get());
   ```
   每次调用 av_read_frame，FFmpeg 内部的 gdigrab 解复用器会用 GDI（BitBlt 等）把屏幕内容抓成一帧原始视频数据，然后打包成 AVPacket 返回给你。

也就是说，**真正的“截屏”动作**是在 FFmpeg 底层的 gdigrab demuxer 里完成的，你的代码只需选好设备并不断调用 av_read_frame() 即可不断获取屏幕帧。

## 重采样（Resampling）  
— 指对音频信号在时域或格式上进行重构，使其满足新的采样率、通道布局或采样格式要求。  

为什么要重采样？  
1. 设备/标准不一致  
   • 不同硬件或协议对采样率（如 48 kHz vs 44.1 kHz）、通道数（单声道/立体声）或样本格式（16 位整型 vs 浮点）有严格要求。  
2. 流程匹配  
   • 采集端输出与编码器/混音器/网络推流的输入必须格式一致，否则会出现抖动、不同步或数据丢失。  
3. 质量优化  
   • 在降采样或升采样时，可以利用滤波器去除超出目标带宽的噪声或避免频谱混叠。  

AudioResampler 的重采样流程  

1. 配置并打开重采样上下文（Open）  
   - av_get_default_channel_layout：根据通道数获取通道布局（channel layout）  
   - swr_alloc：分配 SwrContext  
   - av_opt_set_int：依次设置输入/输出的通道布局、采样率和采样格式（in_channel_layout/in_sample_rate/in_sample_fmt、out_*）  
   - swr_init：初始化上下文并准备好内部滤波器和延迟计算  
   - 保存输入/输出的采样率、通道数、位深和格式信息  

2. 执行重采样（Convert）  
   - swr_get_delay：计算因滤波器和缓冲引入的采样延迟  
   - av_rescale_rnd：根据输入帧样本数和延迟，计算输出帧所需样本数  
   - av_frame_alloc & av_frame_get_buffer：为输出 AVFrame 分配内存并设置其采样率、格式、通道数和样本数  
   - swr_convert：将输入 AVFrame 的样本通过内部滤波器和速率转换，写入到输出帧的 data[] 缓冲中  
   - 更新 out_frame->nb_samples 为实际转换后的样本数，并保留时间戳  

3. 关闭并释放（Close）  
   - swr_is_initialized/swr_close：若上下文已初始化则关闭内部滤波器  
   - swr_context_ 置空，释放所有与重采样相关的资源  

这样，AudioResampler 就能在多种音频参数不匹配的情况下，平滑地把源音频转换成目标格式，确保后续编码、混音或网络传输的正确性和音质。

AudioEncoder 的完整工作流程可分为以下几个阶段：

1. 构造与析构  
   • 构造函数中只是将内部重采样器指针置空，不分配任何资源；  
   • 析构函数中调用 Close()，保证重采样器和编码器上下文被正确释放。

2. 打开编码器（Open）  
   2.1 查找编码器  
     – 调用 avcodec_find_encoder(AV_CODEC_ID_AAC) 获取 AAC 编码器；  
   2.2 分配编码上下文  
     – avcodec_alloc_context3() 创建 AVCodecContext；  
   2.3 配置编码参数  
     – 采样率 (sample_rate)、声道数 (channels)；  
     – 采样格式设为浮点平面格式 (AV_SAMPLE_FMT_FLTP)；  
     – 声道布局由 av_get_default_channel_layout() 得到；  
     – 码率 (bit_rate)；  
     – 设置全局头标志 (AV_CODEC_FLAG_GLOBAL_HEADER)。  
   2.4 打开编码器  
     – avcodec_open2() 初始化编码器，失败则清理并返回 false。  
   2.5 创建重采样器  
     – new AudioResampler()；  
     – 调用 AudioResampler::Open(输入采样率、输入通道数、输入格式，输出采样率、通道数、AV_SAMPLE_FMT_FLTP)；  
     – 如果重采样器初始化失败，同样清理并返回 false。  
   2.6 标记已初始化  
     – is_initialzed_ = true。

3. 编码流程（Encode）  
   每次调用 Encode(pcm, samples) 返回一个 AVPacketPtr：  
   3.1 构造输入帧 AVFrame  
     – av_frame_alloc() 分配帧结构；  
     – 设置 sample_rate、format、channels、channel_layout、nb_samples；  
     – 根据累计的 pts_（累加样本数，并按 time_base 转换）赋值 in_frame->pts；  
   3.2 分配帧缓冲区  
     – av_frame_get_buffer() 为 in_frame 的 data[] 分配内存；  
     – 将用户提供的 PCM 数据 memcpy 到 in_frame->data[0]；  
   3.3 重采样  
     – audio_resampler_->Convert(in_frame, fltp_frame)；  
     – 由原始 PCM → 平面浮点格式，结果保存在 fltp_frame；  
   3.4 送入编码器  
     – avcodec_send_frame(codecContext_, fltp_frame.get())；  
   3.5 接收编码包  
     – av_packet_alloc() 分配 AVPacket； av_init_packet() 初始化；  
     – avcodec_receive_packet() 获取 AAC 数据包；  
     – 若成功则返回包装好的 AVPacketPtr，否则返回 nullptr。

4. 关闭与释放（Close）  
   • 先销毁重采样器：audio_resampler_->Close()、reset()；  
   • 再调用父类 EncodBase::Close()，其内部会关闭并释放 AVCodecContext；  
   • 将 is_initialzed_ 重置为 false，以便后续可能的重开。

——  
通过上述步骤，AudioEncoder 将任意格式的 PCM 原始数据：  
1）通过 AudioResampler 统一转换到编码器支持的平面浮点格式；  
2）按帧送入 AAC 编码器；  
3）输出封装好的 AVPacketPtr（包含 AAC 原始数据），供推流或封装到容器使用。


以下是 H264Encoder 的完整工作流程：

1. 对象构造  
   - H264Encoder 构造函数中创建内部 VideoEncoder 实例（std::unique_ptr<VideoEncoder>），但尚未分配编码资源。

2. 打开编码器（Open）  
   - 调用 H264Encoder::OPen(width, height, framerate, bitrate, format)：  
     1. 填充内部 AVConfig 结构（分辨率、帧率、码率、GOP、像素格式）。  
     2. 转发给 VideoEncoder::Open(config)：  
        • 查找 H.264 编码器（avcodec_find_encoder）。  
        • 分配 AVCodecContext 并设置各项参数（宽高、像素格式、帧率、码率、GOP、全局头标志）。  
        • 打开编码器（avcodec_open2），失败则返回 false。  
     3. 若 VideoEncoder 打开成功，则 H264Encoder::OPen 返回 true，标记可用。

3. 编码一帧图像（Encode）  
   - 调用 H264Encoder::Encode(rgba_buffer, width, height, size, out_frame)：  
     1. 清空输出容器 out_frame。  
     2. 调用 VideoEncoder::Encode(rgba_buffer, width, height, size)：  
        • 将 RGBA 原始像素／大小传入，VideoEncoder 内部：  
          – 构建 AVFrame，赋 pts、像素格式、分辨率和缓存；  
          – 如需，调用 sws_scale 将 RGBA 转为编码器目标像素格式（通常 YUV420P）；  
          – avcodec_send_frame/send_packet、avcodec_receive_packet 收包得到 AVPacketPtr。  
        • 返回 AVPacketPtr（H.264 NALU）。  
     3. 判断是否关键帧（pkt->flags & AV_PKT_FLAG_KEY）：  
        • 若是，将 codecContext->extradata（SPS/PPS）拷贝到输出缓存前端。  
     4. 将 pkt->data（裸 H.264 码流）追加到缓存尾部。  
     5. 将拼好的整帧数据复制到 out_frame 并返回总字节数；失败返回 -1。

4. 获取序列参数集（SPS/PPS）  
   - 调用 H264Encoder::GetSequenceParams(out_buffer, size)：  
     • 直接从 VideoEncoder 的 AVCodecContext->extradata 拷贝，返回实际字节长度。

5. 关闭与释放（Close & 析构）  
   - 调用 H264Encoder::Close()：  
     • 转发给 VideoEncoder::Close()，内部调用 avcodec_close、avcodec_free_context 释放编／解码资源。  
   - 析构函数中再次调用 Close，确保所有资源被正确回收。

通过上述流程，H264Encoder 将输入的 RGBA 原始图像按帧转换、编码为 H.264 NALU，并在关键帧前自动插入 SPS/PPS，以便接收端初始化解码。


VideoConvert（VideoConverter）主要用于在编码或显示前，对视频帧做两方面的“变形”：

1. 像素格式转换  
   比如把解码得到的 AV_PIX_FMT_RGBA、AV_PIX_FMT_YUV420P、AV_PIX_FMT_NV12 等，转换成编码器要求的或渲染器能直接用的格式（常见的 YUV420P、RGBA、RGB24……）。

2. 分辨率缩放／裁剪  
   比如把 1920×1080 的帧缩放到 1280×720，或反之，保证后续处理链中各模块对分辨率的一致要求。

具体转换流程（对应 VideoConverter.cpp）：

1. 打开转换器（Open）  
   • 调用 sws_getContext(输入宽度, 输入高度, 输入像素格式,  
            输出宽度, 输出高度, 输出像素格式,  
            SWS_BICUBIC, …)  
  得到 libswscale 的 SwsContext*；  
   • 并保存目标宽高与像素格式。

2. 分配输出帧缓冲  
   每次 Convert 时：  
   • av_frame_alloc() 分配一个 AVFrame；  
   • 设置其 width/height/format/pts；  
   • av_frame_get_buffer() 在内部根据 linesize 为各 data[i] 分配内存。

3. 执行转换  
   • 调用 sws_scale(swsContext_, in_frame->data, in_frame->linesize,  
            0, in_frame->height,  
            out_frame->data, out_frame->linesize)  
   • libswscale 会对每一行、每一个像素做像素格式重排、分量重采样（YUV ↔ RGB）和缩放插值（双三次或线性），返回实际处理的行数。

4. 关闭转换器（Close）  
   • 调用 sws_freeContext() 释放 SwsContext*，清理内部缓存和滤波状态。

这样，VideoConverter 就把任意输入帧“按需”变成目标尺寸和像素格式的 AVFrame，方便上层编码器或显示模块统一处理。

视频编码的大体流程可以分为以下几个阶段，以本项目中 H264Encoder/VideoEncoder 为例：

1. 参数配置与初始化  
   1.1 在 H264Encoder::Open(...) 中填充 AVConfig：  
       • 宽度、高度、帧率、GOP、码率、输入像素格式（通常为 RGBA）  
   1.2 调用 VideoEncoder::Open(config)：  
       • 查找 H.264 编码器（avcodec_find_encoder）  
       • 分配 AVCodecContext 并设置：  
         – width/height  
         – time_base/framerate  
         – pix_fmt = YUV420P  
         – bit_rate、gop_size、max_b_frames、全局头标志  
         – 低延迟参数（ultrafast、zerolatency）  
       • 打开编码器（avcodec_open2）  

2. 像素格式与分辨率转换  
   2.1 首次调用 VideoEncoder::Encode 时，或当输入分辨率/格式与目标不符时：  
       • 创建 VideoConverter 并调用 sws_getContext( in_w,in_h,in_fmt, out_w,out_h,YUV420P )  
   2.2 对每帧：  
       • 将外部传入的 RGBA 原始数据 memcpy 到一个 AVFrame（rgba_frame_）  
       • 调用 VideoConverter::Convert(rgba_frame_, out_frame_)  
         – libswscale 对行／像素做格式转换（RGBA→YUV420P）和缩放  

3. 编码帧数据  
   3.1 设置 out_frame_->pts（若外部传入则用之，否则内部递增）  
   3.2 送帧到编码器：  
       • avcodec_send_frame(codecContext_, out_frame_.get())  
   3.3 从编码器拉包：  
       • avcodec_receive_packet(codecContext_, pkt) → 得到 AVPacketPtr  

4. 关键帧前插入 SPS/PPS  
   4.1 判断 pkt 是否关键帧（pkt->flags & AV_PKT_FLAG_KEY）  
   4.2 若是，先从 codecContext_->extradata 拷贝 SPS/PPS 到输出缓冲  
   4.3 再将 pkt->data（Nalu 负载）紧跟其后  

5. 输出与容器封装  
   • 将拼接好的字节流写入 std::vector<quint8> out_frame，交由推流或文件写入模块使用  

6. 关闭与释放  
   • H264Encoder::Close → VideoEncoder::Close →  
     – sws_freeContext(swsContext_)  
     – avcodec_close(codecContext_) / avcodec_free_context  
     – av_frame_free / av_packet_free  

7. 外层调用示例  
   ```cpp
   H264Encoder encoder;
   encoder.Open(w, h, fps, bitrate, AV_PIX_FMT_RGBA);
   // 每帧循环：
   vector<quint8> pkt;
   int size = encoder.Encode(rgba_ptr, w, h, rgba_size, pkt);
   if (size > 0) send(pkt.data(), size);
   // 结束后：
   encoder.Close();
   ```

整个流程确保了：  
- 任意分辨率/格式的原始帧 → 对齐 H.264 编码器要求（YUV420P、指定分辨率）  
- 低延迟、高效率的实时编码输出标准 H.264 码流，关键帧前携带 SPS/PPS 方便解码端初始化。

视频编码的完整流程（以 H.264 为例），并尽量用中文术语说明 FFmpeg 中的关键环节：

1. 准备编码参数  
   - 分辨率（宽×高）、帧率（fps）、关键帧间隔（GOP）、码率（bitrate）、输入像素格式（如 RGBA）  

2. 打开 H.264 编码器  
   1) 查找编码器：调用“查找编码器”接口（avcodec_find_encoder）  
   2) 分配编码上下文：调用“分配编码上下文”（avcodec_alloc_context3）  
   3) 配置上下文参数：  
      - 宽度、高度  
      - 时间基/帧率（time_base/framerate）  
      - 像素格式：YUV420P  
      - 码率、关键帧间隔、B 帧数、全局头标志  
      - 低延迟策略：设置“预设”（preset）和“快速调优”（tune=zerolatency）  
   4) 打开编码器：调用“打开编码器”（avcodec_open2），失败则释放并返回错误  

3. 初始化像素格式转换器  （libswsscale）
   - 如果输入像素格式或分辨率与编码器目标不符，调用“创建像素转换上下文”（sws_getContext）  
   - 保存转换后目标宽高和像素格式  

4. 单帧编码流程  
   对每一帧原始像素（如 RGBA 数据）执行：  
   1) 构造原始帧：分配并设置 RGBA AVFrame，复制像素数据  
   2) 像素格式/分辨率转换：调用“执行像素格式转换”（sws_scale），输出 YUV420P AVFrame  
   3) 送帧到编码器：调用“送帧接口”（avcodec_send_frame）  
   4) 拉取编码包：调用“拉取编码包”（avcodec_receive_packet），得到 H.264 码流包 AVPacket  
   5) 关键帧处理：如果是关键帧，先将编码器上下文中的序列参数集（SPS/PPS，即 extradata）拷贝到输出流头部  
   6) 输出裸码流：将 AVPacket 中的 NAL 单元数据追加到应用缓冲或推流管道  

5. 关闭与清理  
   - 释放像素转换上下文：调用 sws_freeContext  
   - 关闭编码器上下文：调用 avcodec_close，然后 avcodec_free_context  
   - 释放所有 AVFrame、AVPacket 及其他资源  

通过以上步骤，原始像素→（格式/分辨率对齐）→送入编码器→输出标准 H.264 码流，实现了从“画面”到“码流”的完整转换。