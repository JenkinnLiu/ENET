// 文件: Timer.h
// 功能: 定义定时器 Timer 及管理队列 TimerQueue，用于添加、移除和调度定时任务

// 引入所需的标准库头文件
#include <map>
#include <unordered_map>
#include <thread>
#include <cstdint>
#include <functional>
#include <chrono>
#include <memory>

// TimerEvent: 定时器到期时调用的回调函数签名，返回 true 则重复执行，返回 false 则执行一次后销毁
typedef std::function<bool(void)> TimerEvent;
// TimerId: 定时器的唯一标识类型
typedef uint32_t TimerId;

// Timer: 表示单个定时器，保存回调函数、触发间隔和下次超时戳
class Timer
{
public:
    // 构造函数：设置回调和触发间隔
    // 参数:
    //   event - 到期时调用的回调函数
    //   mesc  - 触发间隔（毫秒）
    Timer(const TimerEvent& event, uint32_t mesc)
        : evenrt_callbak_(event)
        , interval_(mesc){}

    // 析构函数：无特殊清理操作
    ~Timer(){}

    // Sleep: 阻塞当前线程指定毫秒，用于延时操作
    // 参数: mesc - 阻塞时长（毫秒）
    static void Sleep(uint32_t mesc)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(mesc));
    }

private:
    // 设置下一次触发时间点 = 参数 time_point + interval_
    // time_point 参数通常为当前时间戳
    void SetNextTimeOut(uint64_t time_point)
    {
        next_timeout_ = time_point + interval_;
    }

    // 获取下一次超时触发的时间戳（毫秒）
    int64_t getNextTimeOut()
    {
        return next_timeout_;
    }

private:
    friend class TimerQueue;  // 允许 TimerQueue 访问私有成员

    uint32_t interval_ = 0;          // 触发间隔（毫秒）
    uint64_t next_timeout_ = 0;      // 下次触发时间戳（毫秒级）
    TimerEvent evenrt_callbak_;      // 到期时调用的回调函数
};

// TimerQueue: 管理多个 Timer，提供定时任务的添加、移除和事件处理
class TimerQueue
{
public:
    TimerQueue(){}
    ~TimerQueue(){}

    // AddTimer: 添加新定时器
    // 参数:
    //   event - 到期时调用的回调函数
    //   mesc  - 定时间隔（毫秒）
    // 返回: 分配的 TimerId
    TimerId AddTimer(const TimerEvent& event, uint32_t mesc);

    // RemoveTimer: 根据 TimerId 移除定时器
    // 参数: timerId - 要移除的定时器 ID
    void RemoveTimer(TimerId timerId);

    // HandleTimerEvent: 触发所有到期定时器的回调
    // 返回 true 的重新计算超时并重新排期，false 的移除定时器
    void HandleTimerEvent();

protected:
    // 获取当前系统稳态时钟的毫秒级时间戳，用于调度计算
    int64_t GetTimeNow();

private:
    uint32_t last_timer_id_ = 0;  // 上次分配的定时器 ID，用于生成新 ID
    // 存储所有定时器: ID -> Timer 对象
    std::unordered_map<TimerId, std::shared_ptr<Timer>> timers_;
    // 事件队列: 键为 (超时时间, TimerId)，值为 Timer 对象，可快速获取最早到期的定时器
    std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> events_;
};