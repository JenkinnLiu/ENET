// 文件: TcpConnection.cpp
// 功能: 实现 TcpConnection 类，管理单个连接的读写事件、缓存和生命周期

#include "TcpConnection.h"
#include <unistd.h>
#include "Channel.h"

// 构造函数：初始化缓冲区、Channel，注册回调并在调度器中添加监听
// @param task_schduler 所属 TaskScheduler，用于 IO 事件注册
// @param sockfd        客户端连接的 socket 描述符
TcpConnection::TcpConnection(TaskScheduler *task_schduler, int sockfd)
    : task_schduler_(task_schduler)
    , read_buffer_(new BufferReader())            // 初始化读缓冲区
    , write_buffer_(new BufferWriter(500))        // 初始化写缓冲区，容量为500
    , channel_(new Channel(sockfd))               // 创建用于事件驱动的 Channel
{
    is_closed_ = false;
    // 注册 Channel 读写关闭错误事件的处理函数
    channel_->SetReadCallback([this](){ this->HandleRead(); });
    channel_->SetWriteCallback([this](){ this->HandleWrite(); });
    channel_->SetCloseCallback([this](){ this->HandleClose(); });
    channel_->SetErrorCallback([this](){ this->HandleError(); });

    // 设置 socket 属性：非阻塞、发送缓冲区、心跳保活
    SocketUtil::SetNonBlock(sockfd);
    SocketUtil::SetSendBufSize(sockfd, 100 * 1024);
    SocketUtil::SetKeepAlive(sockfd);

    // 启用读事件，并注册到 TaskScheduler
    channel_->EnableReading();
    task_schduler_->UpdateChannel(channel_);
}

// 析构函数：关闭底层 socket
TcpConnection::~TcpConnection()
{
    int fd = channel_->GetSocket();
    if (fd > 0)
    {
        ::close(fd);
    }
}

// Send(shared_ptr): 零拷贝发送数据
void TcpConnection::Send(std::shared_ptr<char> data, uint32_t size)
{
    if (!is_closed_)
    {
        mutex_.lock();
        write_buffer_->Append(data, size);
        mutex_.unlock();
        this->HandleWrite();  // 立即尝试写
    }
}

// Send(const char*): 拷贝方式发送数据
void TcpConnection::Send(const char *data, uint32_t size)
{
    if (!is_closed_)
    {
        mutex_.lock();
        write_buffer_->Append(data, size);
        mutex_.unlock();
        this->HandleWrite();  // 立即尝试写
    }
}

// DisConnect: 主动断开连接，调用 Close
void TcpConnection::DisConnect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

// HandleRead: 处理可读事件
// 1. 从 socket 读取数据到 read_buffer_
// 2. 调用用户回调 ReadCallback 处理数据，返回 false 则关闭连接
void TcpConnection::HandleRead()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_closed_) { return; }
        int ret = read_buffer_->Read(channel_->GetSocket());
        if (ret < 0)
        {
            this->Close();
            return;
        }
    }
    if (readCb_)
    {
        bool ret = readCb_(shared_from_this(), *read_buffer_);
        if (!ret)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            this->Close();
        }
    }
}

// HandleWrite: 处理可写事件
// 1. 从 write_buffer_ 发送数据
// 2. 根据缓冲区是否为空启用/禁用写事件
void TcpConnection::HandleWrite()
{
    if (is_closed_) { return; }
    if (!mutex_.try_lock()) { return; }

    int ret = write_buffer_->Send(channel_->GetSocket());
    if (ret < 0)
    {
        this->Close();
        mutex_.unlock();
        return;
    }
    bool empty = write_buffer_->IsEmpty();

    if (empty)
    {
        if (channel_->IsWriting())
        {
            channel_->DisableWriting();
            task_schduler_->UpdateChannel(channel_);
        }
    }
    else if (!channel_->IsWriting())
    {
        channel_->EnableWriting();
        task_schduler_->UpdateChannel(channel_);
    }
    mutex_.unlock();
}

// HandleClose: 处理连接挂起/关闭事件
void TcpConnection::HandleClose()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

// HandleError: 处理错误事件，关闭连接
void TcpConnection::HandleError()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

// Close: 统一关闭逻辑
// 1. 标记关闭
// 2. 从 TaskScheduler 移除 Channel
// 3. 调用用户注册的 CloseCallback 和 DisConnectCallback
void TcpConnection::Close()
{
    if (!is_closed_)
    {
        is_closed_ = true;
        task_schduler_->RmoveChannel(channel_);
        if (closeCb_)      { closeCb_(shared_from_this()); }
        if (disconnectCb_) { disconnectCb_(shared_from_this()); }
    }
}
