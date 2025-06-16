#include "EventLoop.h"

EventLoop::EventLoop(uint32_t num_threads)
    :index_(1)
    ,num_threads_(num_threads)
{
    this->Loop();
}

EventLoop::~EventLoop()
{
    this->Quit();
}

std::shared_ptr<TaskScheduler> EventLoop::GetTaskSchduler()
{
    if(task_schdulers_.size() == 1)
    {
        return task_schdulers_.at(0);
    }
    else
    {
        auto task_schduler = task_schdulers_.at(index_);
        index_++;
        if(index_ >= task_schdulers_.size())
        {
            index_ = 0;
        }
        return task_schduler;
    }
    return nullptr;
}

TimerId EventLoop::AddTimer(const TimerEvent &event, uint32_t mesc)
{
    if(task_schdulers_.size() > 0)
    {
        return task_schdulers_[0]->AddTimer(event,mesc);
    }
    return 0;
}

void EventLoop::RemvoTimer(TimerId timerId)
{
    if(task_schdulers_.size() > 0)
    {
        task_schdulers_[0]->RemvoTimer(timerId);
    }
}

void EventLoop::UpdateChannel(ChannelPtr channel)
{
    if(task_schdulers_.size() > 0)
    {
        task_schdulers_[0]->UpdateChannel(channel);
    }
}

void EventLoop::RmoveChannel(ChannelPtr &channel)
{
    if(task_schdulers_.size() > 0)
    {
        task_schdulers_[0]->RmoveChannel(channel);
    }
}

void EventLoop::Loop()
{
    if(!task_schdulers_.empty())
    {
        return ;
    }

    for(uint32_t n = 0; n < num_threads_; n++)
    {
        std::shared_ptr<TaskScheduler> task_schduler_ptr(new SelectTaskScheduler(n));
        task_schdulers_.push_back(task_schduler_ptr);
        std::shared_ptr<std::thread> thread(new std::thread(&TaskScheduler::Start,task_schduler_ptr.get()));
        thread->native_handle();
        threads_.push_back(thread);
    }
}

void EventLoop::Quit()
{
    for(auto iter : task_schdulers_)
    {
        iter->Stop();
    }
    for(auto iter : threads_)
    {
        if(iter->joinable())
        {
            iter->join();
        }
    }
    task_schdulers_.clear();
    threads_.clear();
}
