#include "../EdoyunNet/TcpConnection.h"
#include "define.h"

/**
 * @file LoginConnection.h
 * @brief 登录服务器单个客户端连接处理类
 *
 * 继承自 TcpConnection，负责解析用户注册、登录、注销请求，
 * 调用 ORMManager 进行数据库操作，并返回对应结果给客户端。
 */
class LoginConnection : public TcpConnection
{
public:
    /**
     * @brief 构造函数，设置数据读取回调
     * @param scheduler 异步任务调度器指针
     * @param socket    TCP 连接描述符
     */
    LoginConnection(TaskScheduler* scheduler, int socket);

    /**
     * @brief 析构函数，清理连接状态和资源
     */
    ~LoginConnection();

protected:
    /**
     * @brief 判断请求是否超时
     * @param timestamp 来自请求包的时间戳（秒）
     * @return 如果请求时间与当前时间差超过超时阈值返回 true
     */
    bool IsTimeout(uint64_t timestamp);

    /**
     * @brief 数据到达回调入口，调用 HandleMessage 处理缓冲区中的消息
     * @param buffer 网络接收缓冲区
     * @return 始终返回 true 以保持连接
     */
    bool OnRead(BufferReader& buffer);

    /**
     * @brief 解析并分发单条协议消息到具体处理函数
     * @param buffer 网络接收缓冲区
     */
    void HandleMessage(BufferReader& buffer);

    /**
     * @brief 清理连接状态，关闭资源（目前为空实现）
     */
    void Clear();

private:
    /** @brief 处理用户注册请求(UserRegister 包体) */
    void HandleRegister(const packet_head* data);
    /** @brief 处理用户登录请求(UserLogin 包体) */
    void HandleLogin(const packet_head* data);
    /** @brief 处理用户注销请求(UserDestory 包体) */
    void HandleDestory(const packet_head* data);
};