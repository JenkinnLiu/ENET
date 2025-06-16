#ifndef REMOTEMANAGER_H
#define REMOTEMANAGER_H
#include "RtmpPushManager.h"
#include "PullerWgt.h"
#include "SigConnection.h"


class RemoteManager : public RtmpPushManager
{
public:
    ~RemoteManager();
    RemoteManager();
    RemoteManager(RemoteManager &&) = delete;
    RemoteManager(const RemoteManager &) = delete;
    RemoteManager &operator=(RemoteManager &&) = delete;
    RemoteManager &operator=(const RemoteManager &) = delete;
public:
    void Init(const QString& sigIp,uint16_t port,const QString& code);
    void StartRemote(const QString& sigIp,uint16_t port,const QString& code);
protected:
    void HandleStopStream();
    bool HandleStartStream(const QString& streamAddr);
private:
    void Close();
private:
    std::unique_ptr<PullerWgt> pullerWgt_;
    std::unique_ptr<EventLoop> event_loop_;
    std::shared_ptr<SigConnection> sig_conn_;
};
#endif // REMOTEMANAGER_H
