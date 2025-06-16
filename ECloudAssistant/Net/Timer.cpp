#include "Timer.h"

TimerId TimerQueue::AddTimer(const TimerEvent &event, uint32_t mesc)
{
    int64_t time_point = GetTimeNow();
    TimerId timer_id = ++last_timer_id_;

    auto timer = std::make_shared<Timer>(event,mesc);
    timer->SetNextTimeOut(time_point);
    timers_.emplace(timer_id,timer);
    events_.emplace(std::pair<int64_t,TimerId>(time_point + mesc,timer_id),timer);
    return timer_id;
}

void TimerQueue::RemoveTimer(TimerId timerId)
{
    auto iter = timers_.find(timerId);
    if(iter != timers_.end())
    {
        int64_t timeout = iter->second->getNextTimeOut();
        events_.erase(std::pair<int64_t,TimerId>(timeout,timerId));
        timers_.erase(timerId);
    }
}

void TimerQueue::HandleTimerEvent()
{
    if(!timers_.empty())
    {
        int64_t timepoint = GetTimeNow();
        while(!timers_.empty() && events_.begin()->first.first <= timepoint)
        {
            TimerId timerId = events_.begin()->first.second;
            if(events_.begin()->first.second)
            {
                bool flag = events_.begin()->second->evenrt_callbak_();
                if(flag) //反复执行
                {
                    events_.begin()->second->SetNextTimeOut(timepoint);
                    auto timePtr = std::move(events_.begin()->second);
                    events_.erase(events_.begin());
                    events_.emplace(std::pair<int64_t,TimerId>(timePtr->getNextTimeOut(),timerId),timePtr);
                }
                else //一次性
                {
                    events_.erase(events_.begin());
                    timers_.erase(timerId);
                } 
            }
        }
    }
}

int64_t TimerQueue::GetTimeNow()
{
    auto time_point = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}
