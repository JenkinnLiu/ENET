#include "Acceptor.h"
#include "EventLoop.h"

Acceptor::Acceptor(EventLoop *eventloop)    
    :loop_(eventloop)
    ,tcp_socket_(new TcpSocket())
{
}

Acceptor::~Acceptor()
{
}

int Acceptor::Listen(std::string ip, uint16_t port)
{
    if(tcp_socket_->GetSocket() > 0)
    {
        tcp_socket_->Close();
    }
    int fd = tcp_socket_->Create();
    channelPtr_.reset(new Channel(fd));
    SocketUtil::SetNonBlock(fd);
    SocketUtil::SetReuseAddr(fd);
    SocketUtil::SetReusePort(fd);

    if(!tcp_socket_->Bind(ip,port))
    {
        return -1;
    }

    if(!tcp_socket_->Listen(1024))
    {
        return -2;
    }

    channelPtr_->SetReadCallback([this](){this->OnAccept();});
    channelPtr_->EnableReading();
    loop_->UpdateChannel(channelPtr_);
    return 0;
}

void Acceptor::Close()
{
    if(tcp_socket_->GetSocket() > 0)
    {
        loop_->RmoveChannel(channelPtr_);
        tcp_socket_->Close();
    }
}

void Acceptor::OnAccept()
{
    int fd = tcp_socket_->Accept();
    if(fd > 0)
    {
        if(new_connectCb_)
        {
            new_connectCb_(fd);
        }
    }
}
