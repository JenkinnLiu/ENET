// 文件: Channel.h
// 功能: 封装套接字(Channel)的事件注册与回调分发，支持读、写、关闭、错误等事件处理

#include<functional>
#include<string>
#include<memory>

// 枚举表示不同的事件类型
enum EventType
{
    EVENT_NONE = 0,   // 当前无事件
    EVENT_IN   = 1,   // 可读事件：有数据可读取
    EVENT_PRI  = 2,   // 紧急数据：带外数据到达
    EVENT_OUT  = 4,   // 可写事件：可写数据
    EVENT_ERR  = 8,   // 错误事件：套接字发生错误
    EVENT_HUP  = 16,  // 挂起/关闭事件：连接被挂起或关闭
};

class Channel
{
public:
    typedef std::function<void()> EventCallback; // 事件回调函数类型，无参数无返回值

    // 构造函数：绑定套接字描述符
    Channel(int sockfd);
    ~Channel();

    // 注册不同类型事件的回调函数
    void SetReadCallback(const EventCallback& cb);
    void SetWriteCallback(const EventCallback& cb);
    void SetCloseCallback(const EventCallback& cb);
    void SetErrorCallback(const EventCallback& cb);

    // 查询与修改关注的事件标志
    int GetSocket() const;
    int GetEvents() const;
    void SetEvents(int events);
    void EnableReading();
    void EnableWriting();
    void DisableReading();
    void DisableWriting();
    bool IsNoneEvent() const;
    bool IsWriting() const;
    bool IsReading() const;

    // 事件处理入口，根据内核返回的 events 标志调用对应回调
    void HandleEvent(int events);

private:
    EventCallback read_callback_  = []{};   // 默认空回调，安全调用
    EventCallback write_callback_ = []{};
    EventCallback close_callback_ = []{};
    EventCallback error_callback_ = []{};
    int sockfd_ = 0;    // 绑定的 socket 描述符
    int events_ = 0;    // 当前关注的事件位标志
};

typedef std::shared_ptr<Channel> ChannelPtr;