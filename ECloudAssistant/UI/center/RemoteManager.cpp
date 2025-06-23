/**
 * @file RemoteManager.cpp
 * @brief RemoteManager 类的实现，协调远程控制的主控端与被控端逻辑。
 * @details
 * 实现了 RemoteManager 类的所有功能。这个类是整个远程桌面功能的核心调度器，
 * 它根据被调用的不同方法，来决定当前应用程序实例扮演“主控端”（发起控制方）还是“被控端”（被控制方）的角色。
 * @author [Your Name]
 * @date 2025-06-23
 * @version 1.0
 */

#include "RemoteManager.h"
#include "Net/TcpSocket.h"
#include <QDebug>

/**
 * @brief RemoteManager 析构函数。
 * 
 * 在对象销毁时调用 Close() 方法，确保资源被正确释放。
 */
RemoteManager::~RemoteManager()
{
    Close();
}

/**
 * @brief RemoteManager 构造函数。
 * 
 * 初始化所有成员指针为 nullptr，并创建一个新的 EventLoop 实例。
 * 这个 EventLoop 将用于驱动后续创建的信令连接（SigConnection）的网络事件。
 */
RemoteManager::RemoteManager()
    :pullerWgt_(nullptr)
    ,event_loop_(nullptr)
    ,sig_conn_(nullptr)
{
    // 创建事件循环，参数2表示内部可以管理2个线程
    event_loop_.reset(new EventLoop(2));
}

/**
 * @brief 初始化为“被控端”模式。
 * @param sigIp 信令服务器的 IP 地址。
 * @param port 信令服务器的端口。
 * @param code 本机生成的、用于被主控端连接的唯一连接码。
 * 
 * 该函数执行以下操作：
 * 1. 创建一个 TCP 套接字并连接到信令服务器。
 * 2. 如果连接成功，则创建一个 SigConnection 实例，角色为默认的 PUSH（被控端）。
 * 3. 为信令连接设置“开始推流”和“停止推流”的回调函数。
 * 4. 此后，该实例将等待信令服务器的指令。
 */
void RemoteManager::Init(const QString &sigIp, uint16_t port,const QString& code)
{
    // 连接这个信令服务器
    // 创建tcp连接
    TcpSocket tcp_socket;
    tcp_socket.Create();
    if(!tcp_socket.Connect(sigIp.toStdString(),port))
    {
        qDebug() << "连接信令服务器失败";
        return;
    }
    qDebug() << "连接信令服务器成功";
    // 生成一个信令连接器，角色默认为被控端（Pusher）
    sig_conn_.reset(new SigConnection(event_loop_->GetTaskSchduler().get(),tcp_socket.GetSocket(),code));
    // 设置收到“停止推流”指令时的回调
    sig_conn_->SetStopStreamCallBack([this](){
        this->HandleStopStream();
    });
    // 设置收到“开始推流”指令时的回调
    sig_conn_->SetStartStreamCallBack([this](const QString& streamAddr){
        return this->HandleStartStream(streamAddr);
    });
    return;
}

/**
 * @brief 启动“主控端”模式，发起远程连接。
 * @param sigIp 信令服务器的 IP 地址。
 * @param port 信令服务器的端口。
 * @param code 要连接的被控端的连接码。
 * 
 * 该函数执行以下操作：
 * 1. 创建一个 PullerWgt 实例，这是主控端的UI界面（即AVPlayer窗口）。
 * 2. 显示该窗口。
 * 3. 调用 PullerWgt 的 Connect 方法，由 PullerWgt 内部处理与信令服务器的连接、
 *    发送控制请求以及后续的拉流和播放。
 */
void RemoteManager::StartRemote(const QString &sigIp, uint16_t port, const QString &code)
{
    // 开始远程，创建一个拉流器窗口
    pullerWgt_.reset(new PullerWgt(event_loop_.get(),nullptr));
    pullerWgt_->show();
    // 由拉流器窗口处理后续的连接逻辑
    if(!pullerWgt_->Connect(sigIp,port,code))
    {
        qDebug() << "远程连接失败";
        return;
    }
    qDebug() << "远程连接成功";
}

/**
 * @brief 处理停止推流指令的回调函数。
 * 
 * 当作为被控端时，如果信令连接收到了停止指令，此函数会被调用。
 * 它会调用 RtmpPushManager::Close() 来停止屏幕采集、编码和推流。
 */
void RemoteManager::HandleStopStream()
{
    // 停止推流
    RtmpPushManager::Close();
}

/**
 * @brief 处理开始推流指令的回调函数。
 * @param streamAddr 服务器分配的 RTMP 推流地址。
 * @return bool 成功开启推流返回 true，否则返回 false。
 * 
 * 当作为被控端时，信令服务器会传来一个推流地址。
 * 此函数被调用，并使用该地址调用 RtmpPushManager::Open() 来启动整个推流流程。
 */
bool RemoteManager::HandleStartStream(const QString &streamAddr)
{
    // 开始推流
    qDebug() << "push: " << streamAddr;
    // 调用父类（RtmpPushManager）的方法，开始采集、编码并推流到指定地址
    return this->Open(streamAddr);
}

/**
 * @brief 关闭并清理资源。
 * 
 * 在程序退出或对象销毁时调用。
 * 如果当前是作为被控端（Pusher），则确保停止推流。
 */
void RemoteManager::Close()
{
    // 仅当作为被控端（Pusher）时，sig_conn_才被创建
    if(sig_conn_ && sig_conn_->isPusher())
    {
        // 停止推流
        RtmpPushManager::Close();
    }
    // 如果是主控端，pullerWgt_的析构函数会自动处理资源的释放
}

