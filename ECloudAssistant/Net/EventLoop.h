#include "SelectTaskScheduler.h"
#include <vector>

class EventLoop
{
public:
    EventLoop(uint32_t num_threads = -1);
    ~EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator = (const EventLoop&) = delete;
    std::shared_ptr<TaskScheduler> GetTaskSchduler();
    TimerId AddTimer(const TimerEvent& event,uint32_t mesc);
    void RemvoTimer(TimerId timerId);
    void UpdateChannel(ChannelPtr channel);
    void RmoveChannel(ChannelPtr& channel);
    void Loop();
    void Quit();
private:
    uint32_t num_threads_ = 1;
    uint32_t index_ = 1;
    std::vector<std::shared_ptr<TaskScheduler>> task_schdulers_;
    std::vector<std::shared_ptr<std::thread>> threads_;
};
