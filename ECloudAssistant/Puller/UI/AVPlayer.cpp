/**
 * @file AVPlayer.cpp
 * @brief AVPlayer 类的实现，负责远程桌面的音视频播放和用户交互。
 * @details
 * 实现了 AVPlayer 类的所有功能，包括：
 * - 与信令服务器的连接和通信。
 * - 启动和管理解封装器（AVDEMuxer）以接收和分离音视频流。
 * - 在独立的线程中解码和播放音频（audioPlay）和视频（videoPlay）。
 * - 使用 OpenGL 渲染视频帧，并使用 QAudioOutput 播放音频。
 * - 捕获本地的鼠标和键盘事件，并将其序列化后通过信令连接发送到被控端。
 * - 优雅地处理资源的初始化（Init）和释放（Close）。
 * @author [Your Name]
 * @date 2025-06-23
 * @version 1.0
 */

#include "AVPlayer.h"
#include "AVDEMuxer.h"
#include "Net/EventLoop.h"
#include "Net/SigConnection.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

/**
 * @brief AVPlayer 构造函数。
 * @param loop 事件循环，用于处理底层的网络IO事件。
 * @param parent 父窗口部件。
 */
AVPlayer::AVPlayer(EventLoop* loop, QWidget *parent)
    : OpenGLRender(parent)
    , loop_(loop)
{
    Init();
}

/**
 * @brief AVPlayer 析构函数。
 * 
 * 调用 Close() 确保所有资源被正确释放。
 */
AVPlayer::~AVPlayer()
{
    Close();
}

/**
 * @brief 初始化播放器。
 * 
 * - 创建音视频上下文 `avContext_`。
 * - 创建解封装器 `avDEMuxer_`。
 * - 初始化音频渲染器 `AudioRender`。
 * - 连接视频帧重绘信号 `sig_repaint` 到 `Repaint` 槽。
 */
void AVPlayer::Init()
{
    avContext_ = new AVContext();
    avDEMuxer_ = std::make_unique<AVDEMuxer>(avContext_);
    AudioRender::Init(avContext_->GetAudioDecodeQueue());
    connect(this, &AVPlayer::sig_repaint, this, &OpenGLRender::Repaint);
}

/**
 * @brief 关闭播放器并释放资源。
 * 
 * 这是一个关键的清理函数，确保：
 * - 设置 `stop_` 标志为 true，通知所有线程退出循环。
 * - 等待音频和视频播放线程结束（`join`）。
 * - 停止音频播放设备。
 * - 断开并重置信令连接。
 * - 重置解封装器。
 * - 删除音视频上下文。
 */
void AVPlayer::Close()
{
    stop_ = true;
    if (audioThread_ && audioThread_->joinable()) {
        audioThread_->join();
    }
    if (videoThread_ && videoThread_->joinable()) {
        videoThread_->join();
    }
    AudioRender::Stop();
    if(sig_conn_)
        sig_conn_->DisConnect();
    sig_conn_.reset();
    avDEMuxer_.reset();
    if(avContext_)
        delete avContext_;
    avContext_ = nullptr;
}

/**
 * @brief 连接到信令服务器。
 * @param ip 服务器 IP 地址。
 * @param port 服务器端口。
 * @param code 连接码，用于身份验证。
 * @return bool 连接成功返回 true，否则返回 false。
 */
bool AVPlayer::Connect(QString ip, uint16_t port, QString code)
{
    sig_conn_ = std::make_shared<SigConnection>(loop_, ip.toStdString(), port);
    // 设置信令回调
    sig_conn_->SetStartStreamCallback(std::bind(&AVPlayer::HandleStartStream, this, std::placeholders::_1));
    sig_conn_->SetStopStreamCallback(std::bind(&AVPlayer::HandleStopStream, this, this));
    // 发起连接
    if (sig_conn_->Connect(code.toStdString())) {
        return true;
    }
    return false;
}

/**
 * @brief 处理开始推流的信令回调。
 * @param streamAddr 包含音视频流的地址（例如 RTMP 地址）。
 * @return bool 成功启动返回 true。
 * 
 * 当收到服务器的 `start_stream` 指令时此函数被调用。
 * - 启动解封装器 `avDEMuxer_` 开始从 `streamAddr` 拉流。
 * - 创建并启动音频播放线程 `audioThread_` 和视频播放线程 `videoThread_`。
 */
bool AVPlayer::HandleStartStream(const QString& streamAddr)
{
    if (avDEMuxer_->Open(streamAddr.toStdString().c_str())) {
        audioThread_ = std::make_unique<std::thread>(&AVPlayer::audioPlay, this);
        videoThread_ = std::make_unique<std::thread>(&AVPlayer::videoPlay, this);
        return true;
    }
    return false;
}

/**
 * @brief 处理停止推流的信令回调。
 * 
 * 当收到服务器的 `stop_stream` 指令或连接断开时调用。
 * - 调用 `Close()` 方法来停止所有活动并释放资源。
 */
void AVPlayer::HandleStopStream()
{
    Close();
}

/**
 * @brief 音频播放线程函数。
 * 
 * 在一个循环中运行，直到 `stop_` 标志为 true。
 * - 从音频解码队列中取出 PCM 帧。
 * - 调用 `AudioRender::Write` 将音频数据写入播放设备。
 */
void AVPlayer::audioPlay()
{
    while (!stop_) {
        AVFramePtr frame = avContext_->GetAudioDecodeQueue()->Pop();
        if (frame) {
            AudioRender::Write(frame->data[0], frame->linesize[0]);
        }
    }
}

/**
 * @brief 视频播放线程函数。
 * 
 * 在一个循环中运行，直到 `stop_` 标志为 true。
 * - 从视频解码队列中取出 YUV 帧。
 * - 发射 `sig_repaint` 信号，请求 UI 线程渲染该帧。
 */
void AVPlayer::videoPlay()
{
    while (!stop_) {
        AVFramePtr frame = avContext_->GetVideoDecodeQueue()->Pop(10);
        if (frame) {
            emit sig_repaint(frame);
        }
    }
}

// --- 事件处理函数的实现 ---
// 以下所有事件处理函数都遵循一个模式：
// 1. 捕获 Qt 的 UI 事件 (QMouseEvent, QKeyEvent, QWheelEvent)。
// 2. 将事件的关键信息（如坐标、按键码、滚轮方向）提取出来。
// 3. 调用信令连接对象 `sig_conn_` 的相应方法（如 SendMoustMoveEvent），
//    将这些信息打包成约定的格式，通过信令通道发送给被控端。

void AVPlayer::resizeEvent(QResizeEvent *event)
{
    OpenGLRender::resizeEvent(event);
}

void AVPlayer::wheelEvent(QWheelEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendWheelEvent(event->delta());
}

void AVPlayer::mouseMoveEvent(QMouseEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendMoustMoveEvent(event->x(), event->y());
}

void AVPlayer::mousePressEvent(QMouseEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendMousePressEvent((int)event->button(), event->x(), event->y());
}

void AVPlayer::mouseReleaseEvent(QMouseEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendMouseReleaseEvent((int)event->button(), event->x(), event->y());
}

void AVPlayer::keyPressEvent(QKeyEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendKeyPressEvent(event->key());
}

void AVPlayer::keyReleaseEvent(QKeyEvent *event)
{
    if (sig_conn_)
        sig_conn_->SendKeyReleaseEvent(event->key());
}
