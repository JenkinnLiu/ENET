#include <functional>
#include <memory>
#include "Channel.h"
#include "TcpSocket.h"

class EventLoop;

typedef std::function<void(int)> NewConnectCallback;

class Acceptor
{
public:
    Acceptor(EventLoop* eventloop);
    ~Acceptor();
    inline void SetNewConnectCallback(const NewConnectCallback& cb) {new_connectCb_ = cb;};
    int Listen(std::string ip,uint16_t port);
    void Close();
private:
    void OnAccept();
    EventLoop* loop_ = nullptr;
    ChannelPtr channelPtr_ = nullptr;;
    std::shared_ptr<TcpSocket> tcp_socket_;
    NewConnectCallback new_connectCb_;
};