// 文件: EventLoop.cpp
// 功能: 实现 EventLoop 类，用于创建并管理多个 TaskScheduler 线程，处理定时器和 IO 通道

#include "EventLoop.h"

// 构造函数：初始化线程数量并启动事件循环
// 参数: num_threads - TaskScheduler 线程数，-1 表示使用默认值
EventLoop::EventLoop(uint32_t num_threads)
    : index_(1)
    , num_threads_(num_threads)
{
    this->Loop();  // 启动所有 TaskScheduler 线程
}

// 析构函数：停止所有线程并回收资源
EventLoop::~EventLoop()
{
    this->Quit();  // 通知线程退出并等待回收
}

// GetTaskSchduler: 获取下一个可用的 TaskScheduler 实例，轮询分配
// 返回: 共享指针形式的 TaskScheduler
std::shared_ptr<TaskScheduler> EventLoop::GetTaskSchduler()
{
    if (task_schdulers_.size() == 1) {// 只有一个调度器时直接返回
        return task_schdulers_.at(0);
    } else { // 多个调度器时只获取当前索引
        auto task_schduler = task_schdulers_.at(index_);// 获取当前索引的 TaskScheduler
        index_++;
        if (index_ >= task_schdulers_.size()) {
            index_ = 0;
        }
        return task_schduler;
    }
}

// AddTimer: 将定时任务添加到首个 TaskScheduler
// 参数: event - 定时回调; mesc - 间隔毫秒
// 返回: TimerId
TimerId EventLoop::AddTimer(const TimerEvent &event, uint32_t mesc)
{
    if (!task_schdulers_.empty()) {
        return task_schdulers_[0]->AddTimer(event, mesc);
    }
    return 0;
}

// RemvoTimer: 从首个 TaskScheduler 移除定时任务
// 参数: timerId - 定时器 ID
void EventLoop::RemvoTimer(TimerId timerId)
{
    if (!task_schdulers_.empty()) {
        task_schdulers_[0]->RemvoTimer(timerId);
    }
}

// UpdateChannel: 注册或更新 Channel 到首个 TaskScheduler 进行 IO 监听
// 参数: channel - 待调度的 Channel
void EventLoop::UpdateChannel(ChannelPtr channel)
{
    if (!task_schdulers_.empty()) {
        task_schdulers_[0]->UpdateChannel(channel);
    }
}

// RmoveChannel: 从首个 TaskScheduler 中移除指定 Channel
// 参数: channel - 要移除的 Channel 引用
void EventLoop::RmoveChannel(ChannelPtr &channel)
{
    if (!task_schdulers_.empty()) {
        task_schdulers_[0]->RmoveChannel(channel);
    }
}

// Loop: 创建 num_threads_ 个 EpollTaskScheduler 并启动线程
void EventLoop::Loop()
{
    if (!task_schdulers_.empty()) {
        return;  // 已初始化，直接返回
    }

    for (uint32_t n = 0; n < num_threads_; n++) {// 遍历所有线程，循环创建指定数量的 TaskScheduler
        // 每一个线程创建新的 EpollTaskScheduler 实例
        std::shared_ptr<TaskScheduler> task_schduler_ptr(
            new EpollTaskScheduler(n));
        task_schdulers_.push_back(task_schduler_ptr);
        // 启动线程执行 TaskScheduler::Start
        std::shared_ptr<std::thread> thread(
            new std::thread(&TaskScheduler::Start, task_schduler_ptr.get()));
        threads_.push_back(thread);
    }
}

// Quit: 停止所有 TaskScheduler 并等待线程退出
void EventLoop::Quit()
{
    // 通知所有调度器退出
    for (auto &iter : task_schdulers_) {
        iter->Stop();
    }
    // 等待线程结束并回收
    for (auto &iter : threads_) {
        if (iter->joinable()) {
            iter->join();
        }
    }
    task_schdulers_.clear();
    threads_.clear();
}
