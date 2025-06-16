#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <functional>
#include <memory>

enum EventType
{
	EVENT_NONE   = 0,
	EVENT_IN     = 1,
	EVENT_PRI    = 2,		
	EVENT_OUT    = 4,
	EVENT_ERR    = 8,
	EVENT_HUP    = 16,
};

class Channel 
{
public:
	typedef std::function<void()> EventCallback;
	Channel(int sockfd) 
		: sockfd_(sockfd){}
		
	~Channel() {};
    
	inline void SetReadCallback(const EventCallback& cb)
	{ read_callback_ = cb; }

	inline void SetWriteCallback(const EventCallback& cb)
	{ write_callback_ = cb; }

	inline void SetCloseCallback(const EventCallback& cb)
	{ close_callback_ = cb; }

	inline void SetErrorCallback(const EventCallback& cb)
	{ error_callback_ = cb; }

	inline int GetSocket() const { return sockfd_; }

	inline int  GetEvents() const { return events_; }
	inline void SetEvents(int events) { events_ = events; }
    
	inline void EnableReading() 
	{ events_ |= EVENT_IN; }

	inline void EnableWriting() 
	{ events_ |= EVENT_OUT; }
    
	inline void DisableReading() 
	{ events_ &= ~EVENT_IN; }
    
	inline void DisableWriting() 
	{ events_ &= ~EVENT_OUT; }
       
	inline bool IsNoneEvent() const { return events_ == EVENT_NONE; }
	inline bool IsWriting() const { return (events_ & EVENT_OUT) != 0; }
	inline bool IsReading() const { return (events_ & EVENT_IN) != 0; }
    
	void HandleEvent(int events)
	{	
		if (events & (EVENT_PRI | EVENT_IN)) {
			read_callback_();
		}

		if (events & EVENT_OUT) {
			write_callback_();
		}
        
		if (events & EVENT_HUP) {
			close_callback_();
			return ;
		}

		if (events & (EVENT_ERR)) {
			error_callback_();
		}
	}

private:
	EventCallback read_callback_  = []{};
	EventCallback write_callback_ = []{};
	EventCallback close_callback_ = []{};
	EventCallback error_callback_ = []{};
	int sockfd_ = 0;
	int events_ = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;
#endif