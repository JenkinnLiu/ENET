#ifndef _TCPCONNECTION_H_
#define _TCPCONNECTION_H_
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Channel.h"
#include "TcpSocket.h"
#include "TaskScheduler.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    using DisConnectCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using ReadCallback = std::function<bool(std::shared_ptr<TcpConnection>,BufferReader& buffer)>;
    TcpConnection(TaskScheduler* task_schduler,int sockfd);
    virtual ~TcpConnection();

    inline TaskScheduler* GetTaskSchduler()const{return task_schduler_;}
    inline void SetReadCallback(const ReadCallback& cb){readCb_ = cb;}
    inline void SetCloseCallback(const CloseCallback& cb){closeCb_ = cb;}
    inline bool IsClosed()const{return is_closed_;}
    inline int GetSocket()const{return channel_->GetSocket();}

    void Send(std::shared_ptr<char> data,uint32_t size);
    void Send(const char* data,uint32_t size);
    void DisConnect();
protected:
    virtual void HandleRead();
    virtual void HandleWrite();
    virtual void HandleClose();
    virtual void HandleError();
protected:
    friend class TcpServer;
    bool is_closed_;
    TaskScheduler* task_schduler_;
    void SetDisConnectCallback(const DisConnectCallback& cb) {disconnectCb_ = cb;}
    std::unique_ptr<BufferReader> read_buffer_;
    std::unique_ptr<BufferWriter> write_buffer_;
private:
    void Close();
    std::mutex mutex_;
    std::shared_ptr<Channel> channel_ = nullptr;
    DisConnectCallback disconnectCb_;
    CloseCallback closeCb_;
    ReadCallback readCb_;
};
#endif