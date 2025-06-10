#include <iostream>
#include "../EdoyunNet/EventLoop.h"
#include "RtmpServer.h"

int main()
{
    int count = std::thread::hardware_concurrency();
    EventLoop loop(2);
    auto rtmp_server = RtmpServer::Create(&loop);
    rtmp_server->SetChunkSize(60000);
    rtmp_server->SetEventCallback([](std::string type,std::string stream_path){
        printf("[Event]%s,stream_path%s\n",type.c_str(),stream_path.c_str());
    });
    if(!rtmp_server->Start("192.168.31.30",1935))
    {
        printf("rtmp server failed\n");
    }
    printf("rtmp server success\n");
    //getchar();
    	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
    return 0;
}