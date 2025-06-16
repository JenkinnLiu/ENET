#include "TaskScheduler.h"

class EpollTaskScheduler : public TaskScheduler
{
public:
    EpollTaskScheduler(int id = 0);
    virtual ~EpollTaskScheduler();
    void UpdateChannel(ChannelPtr channel);
    void RmoveChannel(ChannelPtr& channel);
    bool HandleEvent();
protected:
    void Update(int operation,ChannelPtr& Channel);
private:
    int epollfd_ = -1;
    std::mutex mutex_;
    std::unordered_map<int,ChannelPtr> channels_;
};
