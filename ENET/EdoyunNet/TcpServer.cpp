// 文件: TcpServer.cpp
// 功能: 实现 TcpServer 类，管理 Acceptor 监听与 TcpConnection 的创建和生命周期

#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"

// 构造函数：初始化 Acceptor，并注册新连接回调
// 当接收到新客户端连接时，调用 OnConnect 创建 TcpConnection 并加入管理列表
TcpServer::TcpServer(EventLoop *eventloop)
    : loop_(eventloop)
    , port_(0)
    , acceptor_(new Acceptor(eventloop))
{
    // 设置 Acceptor 的新连接回调
    acceptor_->SetNewConnectCallback([this](int fd) {
        // 新建连接，子类可以重写 OnConnect
        TcpConnection::Ptr conn = this->OnConnect(fd); //创建 TcpConnection 对象，子类可重写自定义连接逻辑
        if (conn)
        {
            // 管理连接并监听其断开事件
            this->AddConnection(fd, conn); // 将新连接加入管理 map
            // 注册连接断开回调
            // 当连接断开时，调用 RemoveConnection 移除该连接
            conn->SetDisConnectCallback([this](TcpConnection::Ptr conn) {
                int fd = conn->GetSocket();
                this->RemoveConnection(fd);
            });
        }
    });
}

// 析构函数：停止服务并清理所有连接资源
TcpServer::~TcpServer()
{
    Stop();
}

// Start: 启动 TCP 服务
// 1. 停止已有服务
// 2. 调用 Acceptor::Listen 绑定并监听
// 3. 更新监听状态
// 返回: 服务是否成功启动
bool TcpServer::Start(std::string ip, uint16_t port)
{
    Stop();
    if (!is_stared_)
    {
        if (acceptor_->Listen(ip, port) < 0)
        {
            return false;
        }
        ip_ = ip;
        port_ = port;
        is_stared_ = true;
    }
    return true;
}

// Stop: 停止 TCP 服务
// 1. 对所有已连接的客户端执行断开操作
// 2. 关闭 Acceptor
// 3. 重置启动标记
void TcpServer::Stop()
{
    if (is_stared_)
    {
        for (auto &iter : connects_)
        {
            iter.second->DisConnect();
        }
        acceptor_->Close();
        is_stared_ = false;
    }
}

// OnConnect: 创建 TcpConnection 对象，子类可重写自定义连接逻辑
TcpConnection::Ptr TcpServer::OnConnect(int fd)
{
    return std::make_shared<TcpConnection>(loop_->GetTaskSchduler().get(), fd);
}

// AddConnection: 将新连接加入管理 map
void TcpServer::AddConnection(int fd, TcpConnection::Ptr conn)
{
    connects_.emplace(fd, conn);
}

// RemoveConnection: 从管理 map 中移除指定连接
void TcpServer::RemoveConnection(int fd)
{
    connects_.erase(fd);
}
