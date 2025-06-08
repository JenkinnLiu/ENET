// 文件: EventLoop.h
// 功能: 事件循环管理类，集成定时器和 IO 通道调度，支持多线程 TaskScheduler 负载均衡

#include "EpoolTaskScheduler.h"
#include <vector>

class EventLoop
{
public:
    // 构造函数：初始化 EventLoop，并创建指定数量的 TaskScheduler 线程
    // 参数: num_threads - TaskScheduler 数量，默认为单线程
    EventLoop(uint32_t num_threads = -1);
    
    // 析构函数：停止所有 TaskScheduler 并回收资源
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator = (const EventLoop&) = delete;

    // 获取下一个可用的 TaskScheduler，用于分配 IO 或定时任务
    std::shared_ptr<TaskScheduler> GetTaskSchduler();

    // 添加定时器任务到第一个 TaskScheduler
    TimerId AddTimer(const TimerEvent& event, uint32_t mesc);

    // 移除指定定时器任务
    void RemvoTimer(TimerId timerId);

    // 更新一个 Channel 到第一个 TaskScheduler 进行事件监听
    void UpdateChannel(ChannelPtr channel);

    // 从 TaskScheduler 中移除一个 Channel
    void RmoveChannel(ChannelPtr& channel);

    // 启动 EventLoop：创建线程并运行 TaskScheduler
    void Loop();

    // 停止 EventLoop：通知线程退出并等待回收
    void Quit();

private:
    uint32_t num_threads_ = 1;  // 要创建的 TaskScheduler 线程数
    uint32_t index_ = 1;        // 轮询索引用于负载均衡
    std::vector<std::shared_ptr<TaskScheduler>> task_schdulers_; // 线程池
    std::vector<std::shared_ptr<std::thread>> threads_;           // 线程句柄
};