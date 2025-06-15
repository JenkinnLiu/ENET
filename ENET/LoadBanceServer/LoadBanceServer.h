#ifndef LOADBANCESERVER_H_
#define LOADBANCESERVER_H_
#include "../EdoyunNet/TcpServer.h"
#include "loaddefine.h"

class LoadBanceServer : public TcpServer, public std::enable_shared_from_this<LoadBanceServer>
{
public:
    static std::shared_ptr<LoadBanceServer> Create(EventLoop* eventloop);  //设单例
    ~LoadBanceServer();
private:
    friend class LoadBanceConnection;
    LoadBanceServer(EventLoop* eventloop);
    virtual TcpConnection::Ptr OnConnect(int socket);
	void UpdateMonitor(const int fd, Monitor_body* info);
	Monitor_body* GetMonitorInfo();
private:
    EventLoop* loop_;
    std::mutex mutex_;
    std::map<int, Monitor_body*> monitorInfos_;
};
#endif