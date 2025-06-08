// 文件: TaskScheduler.h
// 功能: 抽象调度器基类，集成定时器和 IO 事件处理框架

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
    // 构造函数：设定调度器 ID
    TaskScheduler(int id = 1);
    // 析构函数：清理资源
    virtual ~TaskScheduler();

    // 启动调度循环：执行定时器事件处理和 IO 事件处理直到 Stop 被调用
    void Start();
    // 停止调度循环
    void Stop();

    // 添加/移除定时器任务
    TimerId AddTimer(const TimerEvent& event, uint32_t mesc);
    void RemvoTimer(TimerId timerId);

    // 更新/移除 IO Channel，具体实现由子类完成
    virtual void UpdateChannel(ChannelPtr channel){};
    virtual void RmoveChannel(ChannelPtr& channel){};
    // 处理 IO 事件，返回是否继续运行
    virtual bool HandleEvent(){return false;}

    // 获取调度器 ID
    inline int GetId() const { return id_; }

private:
    int id_ = 0;                         // 调度器唯一 ID
    std::mutex mutex_;                   // 保护定时器队列等共享资源
    TimerQueue timer_queue_;             // 定时器管理器
    std::atomic_bool is_shutdown_;       // 调度器停止标志
};

#endif