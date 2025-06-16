#include <iostream>
#include "EventLoop.h"
#include "TcpServer.h"

int main()
{
    uint32_t count = std::thread::hardware_concurrency();
    EventLoop loop(2);
    TcpServer* server = new TcpServer(&loop);
    bool status = server->Start("192.168.31.30",4836);
    std::cout << "server start" << std::endl;
    getchar();
    server->Stop();
    std::cout << "server terminate" << std::endl;
    return 0;
}