// 文件: EpollTaskScheduler.cpp
// 功能: 基于 epoll 的 TaskScheduler 实现，负责 IO 通道的多路复用监听与事件分发

#include "EpoolTaskScheduler.h"
#include <sys/epoll.h>
#include <errno.h>
#include <iostream>

// 构造函数：创建 epoll 实例并初始化父类 ID
// 参数：id - 当前调度器的编号
EpollTaskScheduler::EpollTaskScheduler(int id)
    : TaskScheduler(id)
{
    // 创建 epoll 实例，参数为建议的最大监听数，可根据需求调整
    epollfd_ = epoll_create(1024);
}

// 析构函数：清理资源（可选关闭 epollfd_）
EpollTaskScheduler::~EpollTaskScheduler()
{
    // epoll 文件描述符由操作系统回收，也可手动 close(epollfd_)
}

// UpdateChannel: 向 epoll 注册或更新 Channel 监听事件
// 参数：channel - 待添加/修改的 Channel 智能指针
void EpollTaskScheduler::UpdateChannel(ChannelPtr channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int fd = channel->GetSocket();
    // 已在监听列表中
    if (channels_.find(fd) != channels_.end())
    {
        if (channel->IsNoneEvent())
        {
            // 事件已清空，删除 epoll 监听
            Update(EPOLL_CTL_DEL, channel);
            channels_.erase(fd);
        }
        else
        {
            // 修改已有的监听事件
            Update(EPOLL_CTL_MOD, channel);
        }
    }
    else
    {
        if (!channel->IsNoneEvent())
        {
            // 首次添加到监听列表
            channels_.emplace(fd, channel);
            Update(EPOLL_CTL_ADD, channel);
        }
    }
}

// RmoveChannel: 从 epoll 中移除指定 Channel
// 参数：channel - 待移除的 Channel 智能指针引用
void EpollTaskScheduler::RmoveChannel(ChannelPtr &channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int fd = channel->GetSocket();
    if (channels_.find(fd) != channels_.end())
    {
        Update(EPOLL_CTL_DEL, channel);
        channels_.erase(fd);
    }
}

// HandleEvent: 等待 epoll 事件并调用对应 Channel 回调处理
// 返回：true 表示正常或仅遇到 EINTR，可继续；false 表示遇到其他错误
bool EpollTaskScheduler::HandleEvent()
{
    struct epoll_event events[512] = {0};
    // 非阻塞等待，立即返回
    int num_events = epoll_wait(epollfd_, events, 512, 0);
    if (num_events < 0)
    {
        if (errno != EINTR)
        {
            // 非信号中断错误，需要上层处理
            return false;
        }
    }
    // 分发每个就绪事件给对应的 Channel
    for (int n = 0; n < num_events; ++n)
    {
        if (events[n].data.ptr)
        {
            // 通过 data.ptr 获取原始指针并调用 HandleEvent
            static_cast<Channel*>(events[n].data.ptr)->HandleEvent(events[n].events);
        }
    }
    return true;
}

// Update: epoll_ctl 操作封装
// 参数：operation - 操作类型（ADD/MOD/DEL），channel - 对应的 Channel
void EpollTaskScheduler::Update(int operation, ChannelPtr &channel)
{
    struct epoll_event event = {0};
    if (operation != EPOLL_CTL_DEL)
    {
        // 新增或修改时填充 event 数据
        event.data.ptr = channel.get();
        event.events   = channel->GetEvents();
    }

    // 调用 epoll_ctl 执行操作，出错时输出日志
    if (::epoll_ctl(epollfd_, operation, channel->GetSocket(), &event) < 0)
    {
        std::cout << "修改 epoll 事件失败，操作: " << operation << std::endl;
    }
}
