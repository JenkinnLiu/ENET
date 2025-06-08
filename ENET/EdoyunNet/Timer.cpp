// 文件: Timer.cpp
// 功能: 实现 TimerQueue 中定时器的添加、移除和调度逻辑

#include "Timer.h"

// AddTimer: 添加一个新的定时器到队列中
// 参数:
//   event - 定时到期时调用的回调函数，返回 true 表示重复触发，返回 false 表示只触发一次
//   mesc  - 两次触发之间的时间间隔（毫秒）
// 返回值: 分配给该定时器的唯一 TimerId
TimerId TimerQueue::AddTimer(const TimerEvent &event, uint32_t mesc)
{
    // 获取当前时间戳（毫秒）
    int64_t time_point = GetTimeNow();
    // 生成新的定时器 ID
    TimerId timer_id = ++last_timer_id_;

    // 创建 Timer 对象并设置下一次超时时间，超时时间就是当前时间戳加上间隔
    auto timer = std::make_shared<Timer>(event, mesc);
    timer->SetNextTimeOut(time_point);
    
    // 将新定时器添加到Map中，以便按ID快速查找
    timers_.emplace(timer_id, timer);
    // 将事件按(超时时间, TimerId)键值对插入有序Map，便于获取最早触发的定时器
    events_.emplace(std::pair<int64_t, TimerId>(time_point + mesc, timer_id), timer);
    return timer_id;
}

// RemoveTimer: 从队列中移除指定 ID 的定时器
// 参数: timerId - 要移除的定时器 ID
void TimerQueue::RemoveTimer(TimerId timerId)
{
    // 在 timers_ 中查找该定时器
    auto iter = timers_.find(timerId);
    if (iter != timers_.end())
    {
        // 获取该定时器下一次超时时间戳
        int64_t timeout = iter->second->getNextTimeOut();
        // 从 events_ 中根据(超时时间, ID)键删除该事件
        events_.erase(std::pair<int64_t, TimerId>(timeout, timerId));
        // 从 timers_ 中删除该定时器
        timers_.erase(timerId);
    }
}

// HandleTimerEvent: 执行所有已到期的定时器回调
//  - 如果回调返回 true，则重新计算下一次超时并重新插入 events_
//  - 如果回调返回 false，则一次性定时器，直接移除
void TimerQueue::HandleTimerEvent()
{
    // 仅当存在定时器时进行处理
    if (!timers_.empty())
    {
        // 获取当前时间戳
        int64_t timepoint = GetTimeNow();
        // 循环处理所有已到期的任务
        while (!timers_.empty() && events_.begin()->first.first <= timepoint)
        {
            // 从事件队列中取出最先到期的定时器ID
            TimerId timerId = events_.begin()->first.second;
            // 调用回调函数，获取返回值
            bool repeat = events_.begin()->second->evenrt_callbak_();
            if (repeat)
            {
                // 如果需要重复执行，重新设置下一次超时
                events_.begin()->second->SetNextTimeOut(timepoint);
                auto timePtr = std::move(events_.begin()->second);
                events_.erase(events_.begin());
                // 重新插入以更新排序
                events_.emplace(std::pair<int64_t, TimerId>(timePtr->getNextTimeOut(), timerId), timePtr);
            }
            else
            {
                // 一次性定时器，执行完毕后直接移除
                events_.erase(events_.begin());
                timers_.erase(timerId);
            }
        }
    }
}

// GetTimeNow: 获取当前稳态时钟的毫秒级时间戳
// 返回值: 当前时间点距 epoch 的毫秒数
int64_t TimerQueue::GetTimeNow()
{
    auto time_point = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}
