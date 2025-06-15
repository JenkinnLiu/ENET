/**
 * @file SigServer.h
 * @brief 信令服务器入口类，监听客户端连接并创建 SigConnection 会话
 *
 * 继承自 TcpServer，负责接受新的 TCP 连接并为每个连接生成对应的 SigConnection 实例，
 * 以处理信令协议的消息解析与转发。
 */

#include "../EdoyunNet/TcpServer.h"

class SigServer : public TcpServer
{
public:
    /**
     * @brief 静态工厂方法，创建并返回信令服务器实例
     * @param eventloop 用于网络 I/O 和定时任务的事件循环对象
     * @return 配置初始化完成的 SigServer 共享指针
     */
    static std::shared_ptr<SigServer> Create(EventLoop* eventloop);

    /**
     * @brief 析构函数，销毁服务器并释放资源
     */
    ~SigServer();

private:
    /**
     * @brief 私有构造函数，仅通过 Create() 调用
     * @param eventloop 事件循环指针，用于管理网络和定时任务
     */
    SigServer(EventLoop* eventloop);

    /**
     * @brief 新客户端连接回调，创建并返回对应的 SigConnection
     * @param socket 新连接的 TCP socket 描述符
     * @return 新创建的 SigConnection 智能指针，用于处理后续信令交互
     */
    virtual TcpConnection::Ptr OnConnect(int socket) override;

private:
    EventLoop* loop_;   ///< 事件循环指针，用于调度任务和回调
};