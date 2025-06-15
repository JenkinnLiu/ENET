#include "LoadBanceConnection.h"
#include <chrono>

#define TIMEOUT 60

LoadBanceConnection::LoadBanceConnection(std::shared_ptr<LoadBanceServer> loadbance_server, TaskScheduler *task_scheduler, int sockfd)
    :TcpConnection(task_scheduler,sockfd)
    ,loadbance_server_(loadbance_server)
    ,socket_(sockfd)
{
    this->SetReadCallback([this](std::shared_ptr<TcpConnection> conn,BufferReader& buffer){
        return this->OnRead(buffer);
    });

    this->SetDisConnectCallback([this](std::shared_ptr<TcpConnection> conn){
        this->DisConnection();
    });
}

LoadBanceConnection::~LoadBanceConnection()
{

}

void LoadBanceConnection::DisConnection()
{

}

bool LoadBanceConnection::OnRead(BufferReader &buffer)
{
    if(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

bool LoadBanceConnection::IsTimeout(uint64_t timestamp) //true就是超时
{
    //获取当前时间
    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    //计算差值
    uint64_t time = nowTimestamp - timestamp; //系统时间不对，应该是创建linux系统系统时时间没有设好
    return abs(time) > TIMEOUT;
}

void LoadBanceConnection::HandleMessage(BufferReader &buffer)
{
   if(buffer.ReadableBytes() < sizeof(packet_head))
   {
        return ;
   }
   packet_head* head = (packet_head*)buffer.Peek();
   if(buffer.ReadableBytes() < head->len)
   {
        return ;
   }

   switch (head->cmd)
   {
   case Login:
        HnadleLogin(buffer);
    break;
   case Minotor:
        HandleMinoterInfo(buffer);
        break;
   default:
        printf("cmd error\n");
    break;
   }
   buffer.Retrieve(head->len);
}

void LoadBanceConnection::HnadleLogin(BufferReader &buffer)
{
    LoginReply reply;
    Login_Info* info = (Login_Info*)buffer.Peek();
    if(IsTimeout(info->timestamp))
    {
        printf("IsTimeout\n");
        reply.cmd = ERROR;
    }
    else
    {
        //获取ip和端口
        auto server = loadbance_server_.lock();
        if(server)
        {
            Monitor_body* minotor = server->GetMonitorInfo();//我们需要在GetMonitorInfo里面做一个资源算法来分配
            reply.ip = minotor->ip;
            reply.port = minotor->port;
            
        }
        else
        {
             printf("!server\n");
            reply.cmd = ERROR;
        }
    }
    Send((const char*)&reply,reply.len);
}

void LoadBanceConnection::HandleMinoterInfo(BufferReader &buffer)
{
    //处理心跳
    //获取info
    Monitor_body* body = (Monitor_body*)buffer.Peek(); 
    auto server = loadbance_server_.lock();
    if(server)
    {
        server->UpdateMonitor(socket_,body);
    }
}
