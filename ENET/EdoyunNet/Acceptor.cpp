// 文件: Acceptor.cpp
// 功能: 实现 Acceptor 类，用于创建监听 socket、注册可读事件并在有新连接时执行回调

#include "Acceptor.h"
#include "EventLoop.h"

// 构造函数: 将 Acceptor 绑定到指定的 EventLoop
// 参数: eventloop - 事件循环指针，用于在监听 socket 可读时分发事件
Acceptor::Acceptor(EventLoop *eventloop)    
    : loop_(eventloop)
    , tcp_socket_(new TcpSocket())
{
}

// 析构函数: 在对象销毁时确保资源释放
Acceptor::~Acceptor()
{
}

// Listen: 在给定 IP 和端口上启动监听
// 1. 如果已有 socket 打开，先关闭
// 2. 创建新 socket 并设置非阻塞、可重用地址/端口
// 3. 绑定并开始监听
// 4. 使用 Channel 注册可读事件，将 OnAccept 作为回调
// 返回: 0 表示成功，<0 表示不同阶段的失败
int Acceptor::Listen(std::string ip, uint16_t port)
{
    // 如果已有 socket，先关闭
    if (tcp_socket_->GetSocket() > 0)
    {
        tcp_socket_->Close();
    }
    // 创建新的 TCP socket
    int fd = tcp_socket_->Create();
    // 创建 Channel 绑定到新 socket
    channelPtr_.reset(new Channel(fd));
    // 设置 socket 为非阻塞，地址和端口可重用
    SocketUtil::SetNonBlock(fd); 
    SocketUtil::SetReuseAddr(fd);
    SocketUtil::SetReusePort(fd);

    // 绑定地址
    if (!tcp_socket_->Bind(ip, port))
    {
        return -1;
    }
    // 开始监听，backlog 大小为 1024
    if (!tcp_socket_->Listen(1024))
    {
        return -2;
    }

    // 注册 accept 回调，当有新连接可读时触发
    channelPtr_->SetReadCallback([this]() { this->OnAccept(); });
    channelPtr_->EnableReading();
    // 通知 EventLoop 更新 Channel
    loop_->UpdateChannel(channelPtr_);// 将 Channel 添加到事件循环中
    return 0;
}

// Close: 停止监听并从 EventLoop 中移除 Channel
void Acceptor::Close()
{
    if (tcp_socket_->GetSocket() > 0)
    {
        // 从事件循环移除 Channel
        loop_->RmoveChannel(channelPtr_);
        // 关闭底层 socket
        tcp_socket_->Close();
    }
}

// OnAccept: 接收新连接，并调用用户注册的回调
// 当 socket 可读时触发此函数，尝试 accept
void Acceptor::OnAccept()
{
    int fd = tcp_socket_->Accept();
    if (fd > 0)
    {
        // 如果注册了回调，传递新连接的 fd
        if (new_connectCb_)
        {
            new_connectCb_(fd);
        }
    }
}
