#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_
#include <memory>
#include <string>
#include <unordered_map>
#include "TcpConnection.h"

class EventLoop;
class Acceptor;

class TcpServer
{
public:
    TcpServer(EventLoop* eventloop);
    ~TcpServer();
    virtual bool Start(std::string ip,uint16_t port);
    virtual void Stop();
    inline  std::string GetIPAddres()const{return ip_;}
    inline  uint16_t GetPort()const{return port_;}
protected:
    virtual TcpConnection::Ptr OnConnect(int fd);
    virtual void AddConnection(int fd,TcpConnection::Ptr conn);
    virtual void RemoveConnection(int fd);
private:
    EventLoop* loop_;
    uint16_t port_;
    std::string ip_;
    std::unique_ptr<Acceptor> acceptor_;
    bool is_stared_ = false;
    std::unordered_map<int,TcpConnection::Ptr> connects_;
};
#endif