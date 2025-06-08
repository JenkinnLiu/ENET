// 文件: Acceptor.h
// 功能: 封装监听套接字的 accept 逻辑，基于 EventLoop 注册可读事件来处理新连接

#include <functional>
#include <memory>
#include "Channel.h"
#include "TcpSocket.h"

class EventLoop;  // 前向声明，节省依赖

typedef std::function<void(int)> NewConnectCallback;  // 新连接事件回调，参数为新连接的 socket fd

class Acceptor
{
public:
    // 构造: 绑定到指定的事件循环，用于注册读事件
    // @param eventloop 事件循环指针
    Acceptor(EventLoop* eventloop);
    
    // 析构: 清理资源及关闭监听
    ~Acceptor();

    // 设置新连接的回调函数，当有客户端连接时触发
    // @param cb 新连接回调函数，参数为新连接的 socket fd
    inline void SetNewConnectCallback(const NewConnectCallback& cb) { new_connectCb_ = cb; };

    // 启动监听，绑定并监听指定 IP 和端口
    // @param ip 要绑定的本地 IP 地址
    // @param port 要绑定的本地端口
    // @return 0 表示成功，<0 表示失败
    int Listen(std::string ip, uint16_t port);

    // 停止监听并从 EventLoop 中移除相关 Channel
    void Close();

private:
    // 内部回调: 被 Channel 可读事件触发，执行 accept 并回调给用户
    void OnAccept();

    EventLoop* loop_ = nullptr;               // 所属事件循环
    ChannelPtr channelPtr_ = nullptr;         // 监听套接字对应的 Channel
    std::shared_ptr<TcpSocket> tcp_socket_;   // 底层 TCP 套接字对象
    NewConnectCallback new_connectCb_;        // 用户注册的新连接回调
};