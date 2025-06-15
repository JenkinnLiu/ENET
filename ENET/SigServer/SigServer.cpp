#include "SigServer.h"
#include "SigConnection.h"
#include "../EdoyunNet/EventLoop.h"

/**
 * @brief 静态工厂方法，创建并返回信令服务器实例
 * @param eventloop 事件循环指针，用于管理网络 I/O
 * @return 初始化后的 SigServer 共享指针
 */
std::shared_ptr<SigServer> SigServer::Create(EventLoop *eventloop)
{
    std::shared_ptr<SigServer> server(new SigServer(eventloop));
    return server;
}

/**
 * @brief 私有构造函数，初始化 TCP 服务器及事件循环指针
 * @param eventloop 事件循环指针
 */
SigServer::SigServer(EventLoop *eventloop)
    :TcpServer(eventloop)
    ,loop_(eventloop)
{
}

/**
 * @brief 析构函数，清理资源
 */
SigServer::~SigServer()
{
}

/**
 * @brief TCP 新连接回调，为新的连接创建 SigConnection 处理会话
 * @param socket 新连接的 socket 描述符
 * @return 对应的 SigConnection 智能指针
 */
TcpConnection::Ptr SigServer::OnConnect(int socket)
{
    return std::make_shared<SigConnection>(loop_->GetTaskSchduler().get(), socket);
}
