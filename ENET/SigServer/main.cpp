#include "../EdoyunNet/EventLoop.h"
#include "SigServer.h"

int main()
{
    int count = std::thread::hardware_concurrency();
    EventLoop loop(1);
    auto sig_server = SigServer::Create(&loop);
    if(!sig_server->Start("192.168.31.30",6539))
    {
        printf("sig server failed\n");
    }
    printf("sig server success\n");
    getchar();
    return 0;
}

