#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"

TcpServer::TcpServer(EventLoop *eventloop)
    :loop_(eventloop)
    ,port_(0)
    ,acceptor_(new Acceptor(eventloop))
{
    acceptor_->SetNewConnectCallback([this](int fd){
        TcpConnection::Ptr conn = this->OnConnect(fd);
        if(conn)
        {
            this->AddConnection(fd,conn);
            conn->SetDisConnectCallback([this](TcpConnection::Ptr conn){
                int fd = conn->GetSocket();
                this->RemoveConnection(fd);
            });
        }
    });
}

TcpServer::~TcpServer()
{
    Stop();
}

bool TcpServer::Start(std::string ip, uint16_t port)
{
    Stop();
    if(!is_stared_)
    {
        if(acceptor_->Listen(ip,port) < 0)
        {
            return false;
        }
        port_ = port;
        ip_ = ip;
        is_stared_ = true;
    }
    return true;
}

void TcpServer::Stop()
{
    if(is_stared_)
    {
        for(auto iter : connects_)
        {
            iter.second->DisConnect();
        }

        acceptor_->Close();
        is_stared_ = false;
    }
}

TcpConnection::Ptr TcpServer::OnConnect(int fd)
{
    return std::make_shared<TcpConnection>(loop_->GetTaskSchduler().get(),fd);
}

void TcpServer::AddConnection(int fd, TcpConnection::Ptr conn)
{
    connects_.emplace(fd,conn);
}

void TcpServer::RemoveConnection(int fd)
{
    connects_.erase(fd);
}
