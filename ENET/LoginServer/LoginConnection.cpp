#include "LoginConnection.h"
#include "ORMManager.h"
#include <chrono>
#define TIMEOUT 60

/**
 * @brief 构造函数，初始化 TCP 连接并注册读取回调
 */
LoginConnection::LoginConnection(TaskScheduler *scheduler, int socket)
    :TcpConnection(scheduler,socket)
{
    // 设置读取回调
    this->SetReadCallback([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });
}

/**
 * @brief 析构函数，清理连接资源（如有）
 */
LoginConnection::~LoginConnection()
{
    Clear();
}

/**
 * @brief 数据到达回调，检查并处理所有缓冲中的消息
 * @param buffer 接收缓冲区
 * @return 始终返回 true，保持连接
 */
bool LoginConnection::OnRead(BufferReader &buffer)
{
    if (buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

/**
 * @brief 从缓冲区读取完整的协议包，分发到具体命令处理函数
 * @param buffer 接收缓冲区
 */
void LoginConnection::HandleMessage(BufferReader &buffer)
{
    if (buffer.ReadableBytes() < sizeof(packet_head))
        return; // 不足以读取包头

    auto head = reinterpret_cast<packet_head*>(buffer.Peek());
    if (buffer.ReadableBytes() < head->len)
        return; // 包体未接收完整

    switch (head->cmd)
    {
    case Login:
        HandleLogin(head);
        break;
    case Register:
        HandleRegister(head);
        break;
    case Destory:
        HandleDestory(head);
        break;
    default:
        break;
    }
    // 移除已处理数据
    buffer.Retrieve(head->len);
}

/**
 * @brief 判断请求包的时间戳是否超过超时阈值
 * @param timestamp 请求中的时间戳值（秒）
 * @return true 表示超时
 */
bool LoginConnection::IsTimeout(uint64_t timestamp)
{
    auto now = std::chrono::system_clock::now();
    auto nowTs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    int64_t diff = nowTs - static_cast<int64_t>(timestamp);
    return std::llabs(diff) > TIMEOUT;
}

/**
 * @brief 清理函数，释放或重置连接相关状态（目前无额外操作）
 */
void LoginConnection::Clear()
{
    // TODO: 如需释放资源可在此实现
}

/**
 * @brief 处理用户注册请求
 * - 检查时间戳是否超时
 * - 查数据库判断用户是否已存在
 * - 若不存在则插入新用户，否则返回已注册错误
 * - 发送 RegisterResult 应答
 */
void LoginConnection::HandleRegister(const packet_head *data)
{
    auto req = reinterpret_cast<const UserRegister*>(data);
    RegisterResult reply;

    if (IsTimeout(req->timestamp))
    {
        reply.resultCode = REQUEST_TIMEOUT;
    }
    else
    {
        std::string code = req->GetCode();
        MYSQL_ROW row = ORMManager::GetInstance()->UserLogin(code.c_str());
        if (row == nullptr)
        {
            ORMManager::GetInstance()->UserRegister(
                req->GetName().c_str(), req->GetCount().c_str(),
                req->GetPasswd().c_str(), req->GetCode().c_str(),
                "192.168.31.30");
            reply.resultCode = S_OK;
        }
        else
        {
            reply.resultCode = ALREADY_REDISTERED;
        }
    }
    Send(reinterpret_cast<const char*>(&reply), reply.len);
}

/**
 * @brief 处理用户登录请求
 * - 判断请求超时
 * - 查询用户是否存在并检查在线状态
 * - 若未登录则标记在线并返回信令服务器信息
 * - 发送 LoginResult 应答
 */
void LoginConnection::HandleLogin(const packet_head *data)
{
    auto req = reinterpret_cast<const UserLogin*>(data);
    LoginResult reply;

    if (IsTimeout(req->timestamp))
    {
        reply.resultCode = REQUEST_TIMEOUT;
    }
    else
    {
        MYSQL_ROW row = ORMManager::GetInstance()->UserLogin(req->GetCode().c_str());
        if (row == nullptr)
        {
            reply.resultCode = SERVER_ERROR;
        }
        else if (std::atoi(row[4]) != 0)
        {
            reply.resultCode = ALREADY_LOGIN;
        }
        else
        {
            reply.resultCode = S_OK;
            reply.SetIp("192.168.31.30");
            reply.port = 6539;
            // 更新数据库用户在线状态和最近登录时间
            auto nowTs = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            ORMManager::GetInstance()->insertClient(
                row[0], row[1], row[2], row[3], 1, nowTs, "192.168.31.30");
        }
    }
    Send(reinterpret_cast<const char*>(&reply), reply.len);
}

/**
 * @brief 处理用户注销请求，调用 ORMManager 删除用户记录
 */
void LoginConnection::HandleDestory(const packet_head *data)
{
    auto req = reinterpret_cast<const UserDestory*>(data);
    ORMManager::GetInstance()->UserDestroy(req->GetCode().c_str());
}
