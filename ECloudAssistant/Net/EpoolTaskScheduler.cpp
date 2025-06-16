#include "EpoolTaskScheduler.h"
#include <sys/epoll.h>
#include <errno.h>
#include <iostream>

EpollTaskScheduler::EpollTaskScheduler(int id)
    :TaskScheduler(id)
{
    //创建epoll
    epollfd_ = epoll_create(1024);
}

EpollTaskScheduler::~EpollTaskScheduler()
{
}

void EpollTaskScheduler::UpdateChannel(ChannelPtr channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int fd = channel->GetSocket();
    if(channels_.find(fd) != channels_.end())
    {
        if(channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL,channel);
            channels_.erase(fd);
        }
        else{
            Update(EPOLL_CTL_MOD,channel);
        }
    }
    else
    {
        if(!channel->IsNoneEvent())
        {
            channels_.emplace(fd,channel);
            Update(EPOLL_CTL_ADD,channel);
        }
    }
}

void EpollTaskScheduler::RmoveChannel(ChannelPtr &channel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int fd = channel->GetSocket();
    if(channels_.find(fd) != channels_.end())
    {
        Update(EPOLL_CTL_DEL,channel);
        channels_.erase(fd);
    }
}

bool EpollTaskScheduler::HandleEvent()
{
    struct epoll_event events[512] = {0};
    int num_events = -1;

    num_events = epoll_wait(epollfd_,events,512,0);
    if(num_events < 0)
    {
        if(errno != EINTR)
        {
            return false;
        }
    }
    for(int n = 0; n < num_events; n++)
    {
        if(events[n].data.ptr)
        {
            ((Channel*)events[n].data.ptr)->HandleEvent(events[n].events);
        }
    }
    return true;
}

void EpollTaskScheduler::Update(int operation, ChannelPtr &Channel)
{
    struct epoll_event event = {0};
    if(operation != EPOLL_CTL_DEL)
    {
        event.data.ptr = Channel.get();
        event.events = Channel->GetEvents();
    }

    if(::epoll_ctl(epollfd_,operation,Channel->GetSocket(),&event) < 0)
    {
        std::cout << "修改epoll事件失败";
    }
}
