#include "LoadBanceServer.h"
#include "LoadBanceConnection.h"
#include "../EdoyunNet/EventLoop.h"
#include <vector>
#include <algorithm>

std::shared_ptr<LoadBanceServer> LoadBanceServer::Create(EventLoop *eventloop)
{
    std::shared_ptr<LoadBanceServer> server(new LoadBanceServer(eventloop));
    return server;
}

LoadBanceServer::LoadBanceServer(EventLoop *eventloop)
    :TcpServer(eventloop)
    ,loop_(eventloop)
{
}


LoadBanceServer::~LoadBanceServer()
{
    for(auto iter : monitorInfos_)
    {
        if(iter.second)
        {
            delete iter.second;
            iter.second = nullptr;
        }
    }
}

TcpConnection::Ptr LoadBanceServer::OnConnect(int socket)
{
    return std::make_shared<LoadBanceConnection>(shared_from_this(),loop_->GetTaskSchduler().get(),socket);
}

void LoadBanceServer::UpdateMonitor(const int fd, Monitor_body *info)
{
    //更新资源的时候需要加锁
    std::lock_guard<std::mutex> lock(mutex_);
    //更新资源
    monitorInfos_[fd] = info;
}

Monitor_body *LoadBanceServer::GetMonitorInfo()
{
    //获取的时候先排序
    std::lock_guard<std::mutex> lock(mutex_);
    //先将这个map中元素转到vector再来排序
    std::vector<MinotorPair> vec(monitorInfos_.begin(),monitorInfos_.end());
    sort(vec.begin(),vec.end(),CmpByValue());//就会通过这个CmpByValue结构体来去排序
    //以为是从小到大排序，使用这个容器第一个就是最优的服务
    return vec[0].second;
}
