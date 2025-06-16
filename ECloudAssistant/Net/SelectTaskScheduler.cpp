#include "SelectTaskScheduler.h"
#include <forward_list>

SelectTaskScheduler::SelectTaskScheduler(int id)
    :TaskScheduler(id)
{
    FD_ZERO(&fd_read_backup_);
    FD_ZERO(&fd_write_backup_);
    FD_ZERO(&fd_exp_backup_);
}

SelectTaskScheduler::~SelectTaskScheduler()
{

}

void SelectTaskScheduler::UpdateChannel(ChannelPtr channel)
{
    //加锁
    std::lock_guard<std::mutex> lock(mutex_);

    //获取socket
    SOCKET socket = channel->GetSocket();

    //查询这个socket是否已经存在
    if(channels_.find(socket) != channels_.end()) //存在
    {
        if(channel->IsNoneEvent()) //不关心任何事件，就需要移除
        {
            //重置这个事件
            is_fd_read_reset_ = true;
            is_fd_write_reset_ = true;
            is_fd_exp_reset_ = true;
            //移除socket
            channels_.erase(socket);
        }
        else
        {
            is_fd_read_reset_ = true;
            is_fd_write_reset_ = true;
        }
    }
    //没有这个socket，我们去判断事件类型
    else
    {
        if(!channel->IsNoneEvent())
        {
            //添加这个socket
            channels_.emplace(socket,channel);
            //重置这个事件
            is_fd_read_reset_ = true;
            is_fd_write_reset_ = true;
            is_fd_exp_reset_ = true;
        }
    }
}

void SelectTaskScheduler::RemoveChannel(ChannelPtr &channel)
{
    std::lock_guard<std::mutex> lock(mutex_);

    //获取socket
    SOCKET fd = channel->GetSocket();

    //查询socket
    if(channels_.find(fd) != channels_.end())//存在就去移除
    {
        //重置这个事件
        is_fd_read_reset_ = true;
        is_fd_write_reset_ = true;
        is_fd_exp_reset_ = true;
        //移除
        channels_.erase(fd);
    }
}

bool SelectTaskScheduler::HandleEvent()
{
    //准备事件
    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_exp;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_exp);
    bool fd_read_reset = false;
    bool fd_write_reset = false;
    bool fd_exp_reset = false;

    if(is_fd_read_reset_ || is_fd_write_reset_ || is_fd_exp_reset_)
    {
        if(is_fd_exp_reset_)
        {
            maxfd_ = 0;
        }

        //遍历事件
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto iter : channels_)
        {
            //获取事件类型
            int event = iter.second->GetEvents();
            //获取fd
            SOCKET fd = iter.second->GetSocket();

            //判断事件
            if(is_fd_read_reset_ && (event & EVENT_IN))
            {
                FD_SET(fd,&fd_read);
            }

            if(is_fd_write_reset_ && (event & EVENT_OUT))
            {
                FD_SET(fd,&fd_write);
            }

            if(is_fd_read_reset_)
            {
                FD_SET(fd,&fd_exp);
                //更新最大的fd
                if(fd > maxfd_)
                {
                    maxfd_ = fd;
                }
            }
        }
        fd_read_reset = is_fd_read_reset_;
        fd_write_reset = is_fd_write_reset_;
        fd_exp_reset = is_fd_exp_reset_;
        //将这些重置成员变量置为false;
        is_fd_read_reset_ = false;
        is_fd_write_reset_ = false;
        is_fd_exp_reset_ = false;
    }

    //拷贝这些fd_set
    if(fd_read_reset)
    {
        FD_ZERO(&fd_read_backup_);
        memcpy(&fd_read_backup_,&fd_read,sizeof(fd_set));
    }
    else
    {
        memcpy(&fd_read,&fd_read_backup_,sizeof(fd_set));
    }

    if(fd_write_reset)
    {
        FD_ZERO(&fd_write_backup_);
        memcpy(&fd_write_backup_,&fd_write,sizeof(fd_set));
    }
    else
    {
        memcpy(&fd_write,&fd_write_backup_,sizeof(fd_set));
    }

    if(fd_exp_reset)
    {
        FD_ZERO(&fd_exp_backup_);
        memcpy(&fd_exp_backup_,&fd_exp,sizeof(fd_set));
    }
    else
    {
        memcpy(&fd_exp,&fd_exp_backup_,sizeof(fd_set));
    }

    //开始使用select来处理事件
    //准备时间
    struct timeval tv = {0,0};//如果是空，select不会返回，如果是0会立即返回，如果大于0会等待这个时间在返回
    int ret = select(maxfd_ + 1,&fd_read,&fd_write,&fd_exp,&tv);
    if(ret < 0)
    {
        return false;
    }

    //处理事件
    std::forward_list<std::pair<ChannelPtr,int>> event_list; //存放channel和事件
    if(ret > 0)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto iter : channels_) //遍历这个映射表，来获取这些发生事件再去统一处理
        {
            int events = 0;
            SOCKET socket = iter.second->GetSocket();

            //判断事件类型
            if(FD_ISSET(socket,&fd_read))
            {
                events |= EVENT_IN;
            }

            if(FD_ISSET(socket,&fd_write))
            {
                events |= EVENT_OUT;
            }

            if(FD_ISSET(socket,&fd_exp))
            {
                events |= EVENT_HUP;
            }

            if(events != 0) //有事件发生，就去添加到这个event_list
            {
                event_list.emplace_front(iter.second,events);
            }
        }
    }

    //处理事件
    for(auto& iter : event_list)
    {
        iter.first->HandleEvent(iter.second);
    }
    return true;
}


















