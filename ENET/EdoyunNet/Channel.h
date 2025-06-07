#include<functional>
#include<string>
#include<memory>

// 枚举表示不同的事件类型
enum EventType
{
	EVENT_NONE   = 0,    // 当前无事件
	EVENT_IN     = 1,    // 可读事件：有数据可读取
	EVENT_PRI    = 2,    // 紧急数据事件：如带外数据
	EVENT_OUT    = 4,    // 可写事件：可以写数据
	EVENT_ERR    = 8,    // 错误事件：套接字发生错误
	EVENT_HUP    = 16,   // 挂起事件：连接被挂起或关闭
};

class Channel 
{
public:
	typedef std::function<void()> EventCallback; // 定义事件回调函数的类型

	// 构造函数：初始化 Channel 实例与给定的套接字描述符
	Channel(int sockfd) 
		: sockfd_(sockfd){}
		
	// 析构函数：当前不需特殊资源释放
	~Channel() {};
    
	// 设置读取事件的回调函数，用于处理数据可读时的操作
	inline void SetReadCallback(const EventCallback& cb)
	{ read_callback_ = cb; }

	// 设置写入事件的回调函数，用于处理数据可写时的操作
	inline void SetWriteCallback(const EventCallback& cb)
	{ write_callback_ = cb; }

	// 设置关闭事件的回调函数，用于处理连接关闭时的操作
	inline void SetCloseCallback(const EventCallback& cb)
	{ close_callback_ = cb; }

	// 设置错误事件的回调函数，用于处理发生错误时的操作
	inline void SetErrorCallback(const EventCallback& cb)
	{ error_callback_ = cb; }

	// 返回当前 Channel 所关联的套接字描述符
	inline int GetSocket() const { return sockfd_; }

	// 获取当前注册的事件标志
	inline int  GetEvents() const { return events_; }
	// 设置 Channel 关注的事件标志
	inline void SetEvents(int events) { events_ = events; }
    
	// 启用读事件监听（例如有数据可读取）
	inline void EnableReading() 
	{ events_ |= EVENT_IN; }

	// 启用写事件监听（例如可以写数据）
	inline void EnableWriting() 
	{ events_ |= EVENT_OUT; }
    
	// 禁用读事件监听
	inline void DisableReading() 
	{ events_ &= ~EVENT_IN; }
    
	// 禁用写事件监听
	inline void DisableWriting() 
	{ events_ &= ~EVENT_OUT; }
       
	// 检查当前是否未设置任何事件
	inline bool IsNoneEvent() const { return events_ == EVENT_NONE; }
	// 检查是否设置了写事件监听
	inline bool IsWriting() const { return (events_ & EVENT_OUT) != 0; }
	// 检查是否设置了读事件监听
	inline bool IsReading() const { return (events_ & EVENT_IN) != 0; }
    
	// 根据传入的事件标志，调用相应的回调函数处理事件
	void HandleEvent(int events)
	{	
		// 如果有优先处理事件或读事件，则调用读取回调
		if (events & (EVENT_PRI | EVENT_IN)) {
			read_callback_();
		}

		// 如果有写事件，则调用写回调
		if (events & EVENT_OUT) {
			write_callback_();
		}
        
		// 如果有挂起事件，则调用关闭回调，并退出处理
		if (events & EVENT_HUP) {
			close_callback_();
			return ;
		}

		// 如果有错误事件，则调用错误回调
		if (events & (EVENT_ERR)) {
			error_callback_();
		}
	}

private:
	// 默认回调函数为空 lambda，确保调用安全
	EventCallback read_callback_  = []{};
	EventCallback write_callback_ = []{};
	EventCallback close_callback_ = []{};
	EventCallback error_callback_ = []{};
	
	// Channel 关联的套接字描述符
	int sockfd_ = 0;
	// 当前 Channel 注册的事件标志，初始为无事件
	int events_ = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;