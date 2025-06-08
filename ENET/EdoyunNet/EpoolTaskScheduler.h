// 文件: EpollTaskScheduler.h
// 功能: 基于 epoll 的 TaskScheduler 实现，支持 IO 多路复用监听

#include "TaskScheduler.h"

class EpollTaskScheduler : public TaskScheduler
{
public:
    // 构造：创建 epoll 实例并初始化 ID
    EpollTaskScheduler(int id = 0);
    virtual ~EpollTaskScheduler();

    // 注册或更新 IO Channel 到 epoll 监听列表
    void UpdateChannel(ChannelPtr channel) override;
    // 从 epoll 中移除 Channel
    void RmoveChannel(ChannelPtr& channel) override;

    // 等待并处理 epoll 事件
    bool HandleEvent() override;

protected:
    // 内部通用更新方法，根据操作类型添加/修改/删除 epoll 事件
    void Update(int operation, ChannelPtr& channel);

private:
    int epollfd_ = -1;                                      // epoll 文件描述符
    std::mutex mutex_;                                     // 保护 channels_ 的并发访问
    std::unordered_map<int, ChannelPtr> channels_;         // 保存活跃的 Channel
};
