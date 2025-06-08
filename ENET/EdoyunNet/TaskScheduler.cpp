// 文件: TaskScheduler.cpp
// 功能: 基类 TaskScheduler 的实现，提供调度循环、定时器管理和停止控制

#include "TaskScheduler.h"

// 构造函数：初始化 ID 和停止标志
TaskScheduler::TaskScheduler(int id)
    : id_(id)
    , is_shutdown_(false)
{
}

TaskScheduler::~TaskScheduler()
{
}

// Start: 进入调度循环，周期性处理定时器和 IO 事件
void TaskScheduler::Start()
{
    is_shutdown_ = false;
    while (!is_shutdown_)
    {
        // 处理所有到期的定时器事件
        this->timer_queue_.HandleTimerEvent();
        // 处理 IO 事件，具体由子类实现
        this->HandleEvent();
    }
}

// Stop: 设置停止标志，使 Start 循环退出
void TaskScheduler::Stop()
{
    is_shutdown_ = true;
}

// AddTimer: 添加定时器任务，委托给内部 TimerQueue
// 参数: event - 定时回调, mesc - 间隔毫秒
// 返回: 定时器 ID
TimerId TaskScheduler::AddTimer(const TimerEvent &event, uint32_t mesc)
{
    return timer_queue_.AddTimer(event, mesc);
}

// RemvoTimer: 移除指定定时器任务
// 参数: timerId - 定时器 ID
void TaskScheduler::RemvoTimer(TimerId timerId)
{
    timer_queue_.RemoveTimer(timerId);
}
