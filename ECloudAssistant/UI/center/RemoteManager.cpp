#include "RemoteManager.h"
#include <QDebug>

RemoteManager::~RemoteManager()
{
    Close();
}

RemoteManager::RemoteManager()
    :pullerWgt_(nullptr)
    ,event_loop_(nullptr)
    ,sig_conn_(nullptr)
{
    //创建事件循环
    event_loop_.reset(new EventLoop(2));
}

void RemoteManager::Init(const QString &sigIp, uint16_t port,const QString& code)
{
    //连接这个信令服务器
    //创建tcp连接
    TcpSocket tcp_socket;
    tcp_socket.Create();
    if(!tcp_socket.Connect(sigIp.toStdString(),port))
    {
        qDebug() << "连接信令服务器失败";
        return;
    }
    qDebug() << "连接信令服务器成功";
    //生成一个信令连接器
    sig_conn_.reset(new SigConnection(event_loop_->GetTaskSchduler().get(),tcp_socket.GetSocket(),code));//默认被控端
    sig_conn_->SetStopStreamCallBack([this](){
        this->HandleStopStream();
    });
    sig_conn_->SetStartStreamCallBack([this](const QString& streamAddr){
        return this->HandleStartStream(streamAddr);
    });
    return;
}

void RemoteManager::StartRemote(const QString &sigIp, uint16_t port, const QString &code)
{
    //开始远程
    pullerWgt_.reset(new PullerWgt(event_loop_.get(),nullptr));
    pullerWgt_->show();
    //创建一个拉流器开始连接
    if(!pullerWgt_->Connect(sigIp,port,code))
    {
        qDebug() << "远程连接失败";
        return;
    }
    qDebug() << "远程连接成功";
}

void RemoteManager::HandleStopStream()
{
    //停止推流
    RtmpPushManager::Close();
}

bool RemoteManager::HandleStartStream(const QString &streamAddr)
{
    //开始推流
    qDebug() << "push: " << streamAddr;
    return this->Open(streamAddr);
}

void RemoteManager::Close()
{
    if(sig_conn_->isPusher())
    {
        //停止推流
        RtmpPushManager::Close();
    }
}

