// 文件: TcpServer.h
// 功能: 封装 TCP 服务端，基于 Acceptor 接收新连接并管理 TcpConnection

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
    // 构造: 绑定到指定事件循环
    // @param eventloop 事件循环指针
    TcpServer(EventLoop* eventloop);
    ~TcpServer();

    // 启动服务：绑定 IP/端口并开始监听
    // @param ip  要监听的地址
    // @param port 要监听的端口
    // @return 是否成功
    virtual bool Start(std::string ip, uint16_t port);

    // 停止服务：关闭所有连接并停止监听
    virtual void Stop();

    // 获取当前监听的 IP 和端口
    inline std::string GetIPAddres() const { return ip_; }
    inline uint16_t GetPort() const { return port_; }

protected:
    // 新连接回调，子类可重写定制连接的 TcpConnection 对象
    // @param fd 新连接的 socket 描述符
    // @return TcpConnection 智能指针
    virtual TcpConnection::Ptr OnConnect(int fd);

    // 将新建的连接加入管理列表
    virtual void AddConnection(int fd, TcpConnection::Ptr conn);

    // 从管理列表移除断开的连接
    virtual void RemoveConnection(int fd);

private:
    EventLoop* loop_;                         // 所属事件循环
    uint16_t port_;                           // 本地监听端口
    std::string ip_;                          // 本地监听地址
    std::unique_ptr<Acceptor> acceptor_;      // 用于接收新连接的 Acceptor
    bool is_stared_ = false;                  // 标记服务是否已启动
    // 存储所有活动连接: fd -> TcpConnection
    std::unordered_map<int, TcpConnection::Ptr> connects_;
};

#endif // _TCPSERVER_H_