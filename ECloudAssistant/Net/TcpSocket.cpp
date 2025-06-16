#include "TcpSocket.h"

void SocketUtil::SetNonBlock(int sockfd)
{
    unsigned long on = 1;
    ioctlsocket(sockfd,FIONBIO,&on);
}

void SocketUtil::SetBlock(int sockfd)
{
    unsigned long on = 0;
    ioctlsocket(sockfd,FIONBIO,&on);
}

void SocketUtil::SetReuseAddr(int sockfd)
{
    int on = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on));
}

void SocketUtil::SetReusePort(int sockfd)
{
}

void SocketUtil::SetKeepAlive(int sockfd)
{
    int on = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(char*)&on,sizeof(on));
}

void SocketUtil::SetSendBufSize(int sockfd, int size)
{
    setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,(char*)&size,sizeof(size));
}

void SocketUtil::SetRecvBufSize(int sockfd, int size)
{
    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,(char*)&size,sizeof(size));
}

TcpSocket::TcpSocket()
{
}

TcpSocket::~TcpSocket()
{
}

int TcpSocket::Create()
{
    sockfd_ = ::socket(AF_INET,SOCK_STREAM,0);
    return sockfd_;
}

bool TcpSocket::Bind(std::string ip, short port)
{
    if(sockfd_ == -1)
    {
        return false;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if(::bind(sockfd_,(struct sockaddr*)&addr,sizeof(addr)) == -1)
    {
        return false;
    }
    return true;;
}

bool TcpSocket::Listen(int backlog)
{
    if(sockfd_ == -1) 
    {
        return false;
    }
    if(::listen(sockfd_,backlog) == -1)
    {
        return false;
    }
    return true;
}

bool TcpSocket::Connect(std::string ip, uint16_t port, int timeout)
{
    bool is_connect = true;

    struct sockaddr_in addr = {0};
    socklen_t len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //开始连接
    if(::connect(sockfd_,(struct sockaddr*)&addr,len) == -1)
    {
        return false;
    }

    return true;
}

int TcpSocket::Accept()
{
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    return ::accept(sockfd_,(struct sockaddr*)&addr,&addrlen);
}

void TcpSocket::Close()
{
    if(sockfd_ != -1)
    {
        ::closesocket(sockfd_);
        sockfd_ = -1;
    }
}

void TcpSocket::ShutdownWrite()
{
    if(sockfd_ != -1)
    {
        shutdown(sockfd_,1);
        sockfd_ = -1;
    }
}
