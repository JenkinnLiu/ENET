
#include <stdio.h>
#include "../LoginServer/LoginServer.h"
#include "LoadBanceServer.h"
#include "../EdoyunNet/EventLoop.h"

 // 8523
 //启动负载 登陆服务器
int main()
{
    EventLoop loop(1);
    std::shared_ptr<LoginServer> loginServer = nullptr;
    auto loadServer = LoadBanceServer::Create(&loop);
    if(loadServer->Start("192.168.31.30",8523))
    {
        loginServer = LoginServer::Create(&loop);
        if(loginServer->Start("192.168.31.30",9867))
        {
            printf("server start succefful\n");
        }
    }
    getchar();
    return 0;
}