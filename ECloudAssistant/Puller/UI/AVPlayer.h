/**
 * @file AVPlayer.h
 * @brief 定义了 AVPlayer 类，这是一个集成了音视频播放、渲染和远程控制事件处理的综合性窗口部件。
 * @details
 * AVPlayer 继承自 OpenGLRender 以实现硬件加速的视频渲染，并继承自 AudioRender 以处理音频播放。
 * 它负责与信令服务器建立连接（通过 SigConnection），接收音视频流（通过 AVDEMuxer 解封装），
 * 在独立的线程中解码和播放音视频，并捕获用户输入（鼠标、键盘事件）发送到被控端。
 * @author [Your Name]
 * @date 2025-06-23
 * @version 1.0
 */

#ifndef AVPLAYER_H
#define AVPLAYER_H
#include "OpenGLRender.h"
#include "AudioRender.h"
#include "AVDEMuxer.h"

class EventLoop;
class SigConnection;

/**
 * @class AVPlayer
 * @brief 远程桌面播放器窗口，负责音视频流的接收、解码、播放以及用户输入的捕获和发送。
 *
 * 该类是远程控制功能的核心客户端组件。它整合了以下功能：
 * 1.  **视频渲染**: 继承自 `OpenGLRender`，使用 OpenGL 渲染从被控端接收的 YUV 视频帧。
 * 2.  **音频播放**: 继承自 `AudioRender`，播放从被控端接收的 PCM 音频数据。
 * 3.  **信令交互**: 使用 `SigConnection` 与信令服务器通信，处理连接、开始/停止推流等信令。
 * 4.  **流处理**: 使用 `AVDEMuxer` 从网络流中解封装音频和视频包。
 * 5.  **多线程处理**: 在单独的线程中运行音频和视频的播放循环，以避免阻塞 UI 线程。
 * 6.  **事件处理**: 重写 Qt 的事件处理函数（如鼠标、键盘事件），将用户输入打包并通过信令连接发送给被控端。
 */
class AVPlayer : public OpenGLRender ,public AudioRender
{
    Q_OBJECT
public:
    /**
     * @brief 析构函数。
     *
     * 负责调用 Close() 方法释放资源。
     */
    ~AVPlayer();

    /**
     * @brief 构造函数。
     * @param loop 事件循环对象指针，用于网络事件处理。
     * @param parent 父窗口部件指针。
     */
    explicit AVPlayer(EventLoop* loop,QWidget* parent = nullptr);

    /**
     * @brief 连接到信令服务器。
     * @param ip 信令服务器的 IP 地址。
     * @param port 信令服务器的端口号。
     * @param code 用于验证的连接码。
     * @return bool 连接成功返回 true，否则返回 false。
     */
    bool Connect(QString ip,uint16_t port,QString code);

signals:
    /**
     * @brief 信号：请求重绘一帧视频。
     * @param frame 待渲染的视频帧（AVFramePtr）。
     *
     * 当视频播放线程从队列中取出一帧视频时，会发出此信号，
     * 连接到 OpenGLRender::Repaint 槽函数，在 UI 线程中执行渲染。
     */
    void sig_repaint(AVFramePtr frame);

protected:
    /**
     * @brief 音频播放线程的主循环函数。
     *
     * 从音频队列中取出解码后的 PCM 帧并写入音频设备进行播放。
     */
    void audioPlay();

    /**
     * @brief 视频播放线程的主循环函数。
     *
     * 从视频队列中取出解码后的视频帧并发射 sig_repaint 信号。
     */
    void videoPlay();

    /**
     * @brief 初始化播放器资源。
     *
     * 创建 AVContext、AVDEMuxer，初始化音频渲染器，并连接信号槽。
     */
    void Init();

    /**
     * @brief 关闭播放器并释放所有资源。
     *
     * 停止所有线程，重置智能指针，确保资源被正确回收。
     */
    void Close();

    // --- Qt 事件重写 ---

    /** @brief 重写窗口尺寸改变事件，以调整 OpenGL 视口。 */
    virtual void resizeEvent(QResizeEvent *event) override;
    /** @brief 重写鼠标滚轮事件，将滚轮操作发送到被控端。 */
    virtual void wheelEvent(QWheelEvent *event) override;
    /** @brief 重写鼠标移动事件，将鼠标坐标发送到被控端。 */
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    /** @brief 重写鼠标按下事件，将按键信息发送到被控端。 */
    virtual void mousePressEvent(QMouseEvent *event) override;
    /** @brief 重写鼠标释放事件，将按键信息发送到被控端。 */
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    /** @brief 重写键盘按下事件，将按键信息发送到被控端。 */
    virtual void keyPressEvent(QKeyEvent *event) override;
    /** @brief 重写键盘释放事件，将按键信息发送到被控端。 */
    virtual void keyReleaseEvent(QKeyEvent *event) override;

private:
    /**
     * @brief 处理来自信令服务器的停止推流指令的回调函数。
     */
    void HandleStopStream();

    /**
     * @brief 处理来自信令服务器的开始推流指令的回调函数。
     * @param streamAddr 包含音视频流的地址（例如，rtmp://...）。
     * @return bool 成功开始拉流和播放返回 true。
     */
    bool HandleStartStream(const QString& streamAddr);

private:
    bool stop_ = false; ///< 停止标志，用于优雅地终止所有线程循环。
    EventLoop* loop_;   ///< 事件循环，用于处理网络IO事件。
    AVContext* avContext_ = nullptr; ///< 音视频上下文，包含解码器、队列等共享资源。
    std::shared_ptr<SigConnection> sig_conn_; ///< 信令连接对象，负责与服务器的信令交互。
    std::unique_ptr<AVDEMuxer> avDEMuxer_ = nullptr; ///< 解封装器，用于从流中分离音频和视频。
    std::unique_ptr<std::thread> audioThread_ = nullptr; ///< 音频播放线程。
    std::unique_ptr<std::thread> videoThread_ = nullptr; ///< 视频播放线程。
};
#endif // AVPLAYER_H
