// 文件: TcpConnection.h
// 功能: 封装单个 TCP 连接，管理 Socket 事件分发、读写缓冲区、以及连接生命周期控制

#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_

#include "BufferReader.h"
#include "BufferWriter.h"
#include "Channel.h"
#include "TcpSocket.h"
#include "TaskScheduler.h"

// TcpConnection: 表示一个客户端连接
//  - 通过 Channel 注册可读、可写、关闭、错误等事件
//  - 使用 BufferReader/BufferWriter 支持异步分片读写
//  - 提供 Send 接口和 ReadCallback、CloseCallback、DisConnectCallback
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    // 断开连接回调: 当连接主动或被动断开时触发
    using DisConnectCallback = std::function<void(Ptr)>;
    // 关闭连接回调: 当连接关闭事件触发时调用
    using CloseCallback     = std::function<void(Ptr)>;
    // 可读事件回调: 处理读取到的数据，返回 false 则关闭连接
    using ReadCallback      = std::function<bool(Ptr, BufferReader&)>;

    // 构造函数: 初始化调度器、缓冲区和事件通道
    // @param task_schduler 所属 TaskScheduler，用于注册 IO 事件
    // @param sockfd        已连接的客户端 socket 描述符
    TcpConnection(TaskScheduler* task_schduler, int sockfd);

    // 析构函数: 关闭底层 socket（若仍打开）并清理资源
    virtual ~TcpConnection();

    // 获取所属调度器
    inline TaskScheduler* GetTaskSchduler() const { return task_schduler_; }
    // 注册读取事件回调
    inline void SetReadCallback(const ReadCallback& cb) { readCb_ = cb; }
    // 注册关闭事件回调
    inline void SetCloseCallback(const CloseCallback& cb) { closeCb_ = cb; }
    // 判断连接是否已关闭
    inline bool IsClosed() const { return is_closed_; }
    // 获取套接字描述符
    inline int GetSocket() const { return channel_->GetSocket(); }

    // 发送数据: 支持零拷贝和拷贝两种重载
    void Send(std::shared_ptr<char> data, uint32_t size);
    void Send(const char* data, uint32_t size);
    // 主动断开连接
    void DisConnect();

protected:
    // 事件处理回调: 由 Channel 在对应事件发生时触发
    virtual void HandleRead();   // 处理可读事件
    virtual void HandleWrite();  // 处理可写事件
    virtual void HandleClose();  // 处理挂起/关闭事件
    virtual void HandleError();  // 处理错误事件

    friend class TcpServer;  // 允许 TcpServer 设置断开回调

    bool is_closed_;                         // 是否已经关闭
    TaskScheduler* task_schduler_;          // IO 和定时调度器
    void SetDisConnectCallback(const DisConnectCallback& cb) { disconnectCb_ = cb; }
    std::unique_ptr<BufferReader> read_buffer_;   // 接收缓冲区
    std::unique_ptr<BufferWriter> write_buffer_; // 发送缓冲区

private:
    // 真正的关闭逻辑: 注销事件、调用回调、设置标志
    void Close();
    std::mutex mutex_;                    // 保护 write_buffer_ 与状态
    std::shared_ptr<Channel> channel_;    // IO 事件分发通道
    DisConnectCallback disconnectCb_;     // 应用层断开回调
    CloseCallback closeCb_;               // 关闭回调
    ReadCallback readCb_;                 // 读取数据回调
};

#endif // _TCPCONNECTION_H_