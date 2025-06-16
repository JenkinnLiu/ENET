#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_
#include <string>
#include <winsock2.h>
#include <windows.h>
#include<ws2tcpip.h>
#include <iphlpapi.h>

class SocketUtil
{
public:
    static void SetNonBlock(int sockfd);
    static void SetBlock(int sockfd);
    static void SetReuseAddr(int sockfd);
    static void SetReusePort(int sockfd);
    static void SetKeepAlive(int sockfd);
    static void SetSendBufSize(int sockfd, int size);
    static void SetRecvBufSize(int sockfd, int size);
};

class TcpSocket
{
public:
    TcpSocket();
    virtual ~TcpSocket();
    int Create();
    bool Bind(std::string ip, short port);
    bool Listen(int backlog);
    bool Connect(std::string ip,uint16_t port,int timeout = 0);
    int  Accept();
    void Close();
    void ShutdownWrite();
    int GetSocket() const { return sockfd_; }
private:
    int sockfd_ = -1;
};
#endif
