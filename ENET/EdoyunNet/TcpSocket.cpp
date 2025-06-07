#include "TcpSocket.h"
#include<fcntl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

void SocketUtil::SetNonBlock(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);// 获取当前文件描述符的状态标志
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);// 设置文件描述符为非阻塞模式
}

void SocketUtil::SetBlock(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK)); // 设置文件描述符为阻塞模式

}

void SocketUtil::SetReuseAddr(int sockfd)
{
    int on = 1;
    //参数：套接字描述符，重用协议，重用地址，选项值，选项值长度
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)); // 设置地址重用选项
}

void SocketUtil::SetReusePort(int sockfd)
{
    int on = 1;
    //参数：套接字描述符，协议，重用端口，选项值，选项值长度
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&on, sizeof(on)); // 设置端口重用选项
}

void SocketUtil::SetKeepAlive(int sockfd)//TCP保活机制
{
    int on = 1;
    //参数：套接字描述符，协议，保持活动选项，选项值，选项值长度
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on)); // 设置保持活动选项
}

void SocketUtil::SetSendBufSize(int sockfd, int size)
{
    //参数：套接字描述符，协议，发送缓冲区大小选项，选项值，选项值长度
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size)); // 设置发送缓冲区大小
}

void SocketUtil::SetRecvBufSize(int sockfd, int size)
{
    //参数：套接字描述符，协议，接收缓冲区大小选项，选项值，选项值长度
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size)); // 设置接收缓冲区大小
}

TcpSocket::TcpSocket()
{
}

TcpSocket::~TcpSocket()
{
}

int TcpSocket::Create()
{
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0); // 创建一个 TCP 套接字
    return sockfd_; // 返回套接字描述符
}

bool TcpSocket::Bind(std::string ip, short port)
{
    if(sockfd_ < 0) // 检查套接字是否有效
    {
        return false; // 套接字无效，返回失败
    }
    struct sockaddr_in addr = {0}; // 初始化地址结构
    addr.sin_family = AF_INET; // IPv4 地址族
    addr.sin_port = htons(port); // 设置端口号，使用网络字节序
    addr.sin_addr.s_addr = inet_addr(ip.c_str()); // 将 IP 地址转换为网络字节序
    // 绑定套接字到指定的 IP 地址和端口
    if (::bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        return false; // 绑定失败
    }
    else
    {
        return true; // 绑定成功
    }
    return false;
}

bool TcpSocket::Listen(int backlog)
{
    if (sockfd_ < 0) // 检查套接字是否有效
    {
        return false; // 套接字无效，返回失败
    }
    // 设置套接字为监听模式，允许接受传入连接
    if (::listen(sockfd_, backlog) == -1)
    {
        return false; // 监听失败
    }
    else {
        return true; // 监听成功
    }
    return false;
}

int TcpSocket::Accept()
{
    struct sockaddr_in addr = { 0 }; // 初始化地址结构
    socklen_t len = sizeof(addr); // 地址结构长度
    // 接受传入连接，返回新连接的套接字描述符
    return ::accept(sockfd_, (struct sockaddr*)&addr, &len);
}

void TcpSocket::Close()
{
    if(sockfd_ != -1) // 检查套接字是否有效
    {
        ::close(sockfd_); // 关闭套接字
        sockfd_ = -1; // 设置套接字描述符为无效值
    }
}

void TcpSocket::ShutdownWrite()
{
    if(sockfd_ != -1) // 检查套接字是否有效
    {
        ::shutdown(sockfd_, SHUT_WR); // 禁止写操作
    }
}
