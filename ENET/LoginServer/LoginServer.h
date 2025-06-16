#include "../EdoyunNet/TcpServer.h"
#include "TcpClient.h"

class LoginServer : public TcpServer
{
public:
   static std::shared_ptr<LoginServer> Create(EventLoop* eventloop); 
   ~LoginServer();
private:
   TimerId id_;
   EventLoop* loop_;
   LoginServer(EventLoop* eventloop);
   virtual TcpConnection::Ptr OnConnect(int socket); 
   std::unique_ptr<TcpClient> client_;
};