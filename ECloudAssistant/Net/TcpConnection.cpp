#include "TcpConnection.h"
#include <unistd.h>
#include "Channel.h"

TcpConnection::TcpConnection(TaskScheduler *task_schduler, int sockfd)
    :task_schduler_(task_schduler)
    ,read_buffer_(new BufferReader())
    ,write_buffer_(new BufferWriter(500))
    ,channel_(new Channel(sockfd))
{
    is_closed_ = false;
    channel_->SetReadCallback([this](){this->HandleRead();});
    channel_->SetWriteCallback([this](){this->HandleWrite();});
    channel_->SetCloseCallback([this](){this->HandleClose();});
    channel_->SetErrorCallback([this](){this->HandleError();});

    //设置套接字属性
    SocketUtil::SetNonBlock(sockfd);
    SocketUtil::SetSendBufSize(sockfd,100 * 1024);
    SocketUtil::SetKeepAlive(sockfd);

    channel_->EnableReading();
    task_schduler_->UpdateChannel(channel_);    
}

TcpConnection::~TcpConnection()
{
    int fd = channel_->GetSocket();
    if(fd > 0)
    {
        ::close(fd);
    }
}

void TcpConnection::Send(std::shared_ptr<char> data, uint32_t size)
{
    if(!is_closed_)
    {
        mutex_.lock();
        write_buffer_->Append(data,size);
        mutex_.unlock();
        this->HandleWrite();
    }
}

void TcpConnection::Send(const char *data, uint32_t size)
{
    if(!is_closed_)
    {
        mutex_.lock();
        write_buffer_->Append(data,size);
        mutex_.unlock();
        this->HandleWrite();
    }
}

void TcpConnection::DisConnect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::HandleRead()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(is_closed_)
        {
            return;
   	    }
        int ret = read_buffer_->Read(channel_->GetSocket());
        if(ret < 0)
   	    {
            this->Close();
            return;
        } //这个锁会释放掉
    }
    if(readCb_)
    {
        bool ret = readCb_(shared_from_this(),*read_buffer_);
        if(!ret)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            this->Close(); //关闭连接
        }
    }
}

void TcpConnection::HandleWrite()
{
    if(is_closed_)
    {
        return;
    }
    //获取锁
    if(!mutex_.try_lock())
    {
        return ;
    }
    int ret = 0;
    bool empty = false;
    do
    {
        ret = write_buffer_->Send(channel_->GetSocket());
        if(ret < 0)
        {
            this->Close();
            mutex_.unlock();
            return;
        }
        empty = write_buffer_->IsEmpty();
    }while (0);

    if(empty)
    {
        if(channel_->IsWriting())
        {
            channel_->DisableWriting();
            task_schduler_->UpdateChannel(channel_);
        }
    }
    else if(!channel_->IsWriting())
    {
        channel_->EnableWriting();
        task_schduler_->UpdateChannel(channel_);
    }
    mutex_.unlock();
}

void TcpConnection::HandleClose()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::HandleError()
{
    std::lock_guard<std::mutex> lock(mutex_);
    this->Close();
}

void TcpConnection::Close()
{
    if(!is_closed_)
    {
        is_closed_ = true;
        task_schduler_->RmoveChannel(channel_);
        if(closeCb_)
        {
            closeCb_(shared_from_this());
        }
        if(disconnectCb_)
        {
            disconnectCb_(shared_from_this());
        }
    }
}
