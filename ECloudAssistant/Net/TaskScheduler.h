#ifndef _TASKSCHDULER_H_
#define _TASKSCHDULER_H_
#include <cstdint>
#include "Timer.h"
#include "Channel.h"
#include <atomic>
#include <mutex>

class TaskScheduler
{
public:
    TaskScheduler(int id = 1);
    virtual ~TaskScheduler();
    void Start();
    void Stop();
    TimerId AddTimer(const TimerEvent& event,uint32_t mesc);
    void RemvoTimer(TimerId timerId);
    virtual void UpdateChannel(ChannelPtr channel){};
    virtual void RmoveChannel(ChannelPtr& channel){};
    virtual bool HandleEvent(){return false;}
    inline int GetId()const{return id_;}
private:
    int id_ = 0;
    std::mutex mutex_;
    TimerQueue timer_queue_;
    std::atomic_bool is_shutdown_;
};
#endif