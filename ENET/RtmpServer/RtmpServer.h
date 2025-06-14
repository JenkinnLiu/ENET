#include <mutex>
#include "../EdoyunNet/TcpServer.h"
#include "rtmp.h"
#include "RtmpSession.h"

/**
 * @class RtmpServer
 * @brief 基于 TCP 的 RTMP 服务端，负责监听客户端连接并管理 RTMP 会话与流。
 *
 * 继承自 TcpServer 提供网络连接监听与分发，继承 Rtmp 提供协议配置参数，
 * 通过 RtmpSession 管理不同流的发布者和订阅者，支持事件回调机制。
 */
class RtmpServer : public TcpServer , public Rtmp, public std::enable_shared_from_this<RtmpServer>
{
public:
    using EventCallback = std::function<void(std::string type, std::string streampath)>;

    /**
     * @brief 创建并返回一个 RTMP 服务实例
     * @param eventloop 用于网络事件循环与定时器管理的 EventLoop 对象
     * @return RTMP 服务共享指针
     */
    static std::shared_ptr<RtmpServer> Create(EventLoop* eventloop);

    /**
     * @brief 析构函数，清理所有资源与会话
     */
    ~RtmpServer();

    /**
     * @brief 注册外部事件回调，当流的发布或停止时触发
     * @param cb 回调函数，参数为事件类型("publish"/"close")和流路径
     */
    void SetEventCallback(const EventCallback& cb);

private:
    friend class RtmpConnection;

    /**
     * @brief 私有构造，仅通过 Create() 调用
     * @param eventloop 事件循环对象
     */
    RtmpServer(EventLoop* eventloop);

    /**
     * @brief 创建新的 RtmpSession，如果不存在则插入会话表
     * @param stream_path 流路径，格式: "/app/streamName"
     */
    void AddSesion(std::string stream_path);

    /**
     * @brief 删除指定流的会话，释放资源
     * @param stream_path 待删除的流路径
     */
    void RemoveSession(std::string stream_path);

    /**
     * @brief 获取或创建流对应的会话
     * @param stream_path 流路径
     * @return 对应的 RtmpSession 智能指针
     */
    RtmpSession::Ptr GetSession(std::string stream_path);

    /**
     * @brief 判断是否已有发布者在推送此流
     * @param stream_path 流路径
     * @return true 表示已有发布者，false 表示无
     */
    bool HasPublisher(std::string stream_path);

    /**
     * @brief 检查是否存在指定流的会话
     * @param stream_path 流路径
     * @return true 表示存在会话，false 否则
     */
    bool HasSession(std::string stream_path);

    /**
     * @brief 调用所有注册的事件回调，通知上层事件
     * @param type 事件类型
     * @param stream_path 事件对应的流路径
     */
    void NotifyEvent(std::string type, std::string stream_path);

    /**
     * @brief 当有新 TCP 连接时创建对应的 RtmpConnection
     * @param socket 新连接的 socket 描述符
     * @return 新创建的 TcpConnection 智能指针
     */
    virtual TcpConnection::Ptr OnConnect(int socket) override;

    EventLoop* loop_;                                ///< 事件循环，用于定时与网络回调
    std::mutex mutex_;                               ///< 保护会话表与回调列表的互斥锁
    std::unordered_map<std::string,RtmpSession::Ptr> rtmp_sessions_;  ///< 流路径到会话的映射表
    std::vector<EventCallback> event_callbacks_;     ///< 注册的事件回调列表
};