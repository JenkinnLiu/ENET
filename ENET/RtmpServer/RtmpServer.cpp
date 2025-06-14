#include "RtmpServer.h"
#include "../EdoyunNet/EventLoop.h"
#include "RtmpConnection.h"

/**
 * @brief 静态工厂方法，创建并返回 RTMP 服务实例
 * @param eventloop 用于网络 I/O 和定时任务的事件循环对象
 * @return 初始化后的 RtmpServer 共享指针
 */
std::shared_ptr<RtmpServer> RtmpServer::Create(EventLoop *eventloop)
{
    std::shared_ptr<RtmpServer> server(new RtmpServer(eventloop));
    return server;
}

/**
 * @brief 私有构造函数，初始化 TCP 服务与定时清理无客户端的会话
 * @param eventloop 事件循环指针
 */
RtmpServer::RtmpServer(EventLoop* eventloop)
    :TcpServer(eventloop)
    ,loop_(eventloop)
    ,event_callbacks_(10)
{
    // 定时移除无订阅者的会话
    loop_->AddTimer([this](){
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto iter = rtmp_sessions_.begin(); iter != rtmp_sessions_.end();){
            if(iter->second->GetClients() == 0)
            {
                rtmp_sessions_.erase(iter++);
            }
            else
            {
                ++iter;
            }
        }
        return true;
    },3000); // 每 3 秒执行一次
}

/**
 * @brief 析构函数，销毁服务并释放所有会话资源
 */
RtmpServer::~RtmpServer()
{
}

/**
 * @brief 注册流发布/停止事件回调
 * @param cb 回调函数，参数为事件类型和流路径
 */
void RtmpServer::SetEventCallback(const EventCallback &cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    event_callbacks_.push_back(cb);
}

/**
 * @brief 创建新的会话，如果不存在则插入，会在发布(push)阶段调用
 * @param stream_path 流路径，格式: "/app/streamName"
 */
void RtmpServer::AddSesion(std::string stream_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(rtmp_sessions_.find(stream_path) == rtmp_sessions_.end())
    {
        rtmp_sessions_[stream_path] = std::make_shared<RtmpSession>();
    }
}

/**
 * @brief 删除指定流的会话，在删除流(deleteStream)时调用
 * @param stream_path 流路径
 */
void RtmpServer::RemoveSession(std::string stream_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rtmp_sessions_.erase(stream_path);
}

/**
 * @brief 获取或创建指定流路径对应的会话
 * @param stream_path 流路径
 * @return 对应的 RtmpSession 智能指针
 */
RtmpSession::Ptr RtmpServer::GetSession(std::string stream_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(rtmp_sessions_.find(stream_path) == rtmp_sessions_.end())
    {
        rtmp_sessions_[stream_path] = std::make_shared<RtmpSession>();
    }
    return rtmp_sessions_[stream_path];
}

/**
 * @brief 判断指定流是否已有发布者
 * @param stream_path 流路径
 * @return true 表示已有 Publisher，false 表示无
 */
bool RtmpServer::HasPublisher(std::string stream_path)
{
    auto session = GetSession(stream_path);
    if(!session)
    {
        return false;
    }
    return (session->GetPublisher() != nullptr);
}

/**
 * @brief 判断指定流路径对应的会话是否已存在
 * @param stream_path 流路径
 * @return true 表示已存在会话，false 表示无
 */
bool RtmpServer::HasSession(std::string stream_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return (rtmp_sessions_.find(stream_path) != rtmp_sessions_.end());
}

/**
 * @brief 通知所有注册的回调函数，触发发布或关闭事件
 * @param type 事件类型，如 "publish" 或 "close"
 * @param stream_path 事件对应的流路径
 */
void RtmpServer::NotifyEvent(std::string type, std::string stream_path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for(auto &event_cb : event_callbacks_)
    {
        if(event_cb)
        {
            event_cb(type, stream_path);
        }
    }
}

/**
 * @brief TCP 新连接回调，创建 RtmpConnection 处理该连接
 * @param socket 新连接的 socket 描述符
 * @return 对应的 RtmpConnection 智能指针
 */
TcpConnection::Ptr RtmpServer::OnConnect(int socket)
{
    return std::make_shared<RtmpConnection>(shared_from_this(), loop_->GetTaskSchduler().get(), socket);
}
