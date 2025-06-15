#include "SigConnection.h"
#include "ConnectionManager.h"
#include <algorithm>

/**
 * @brief 构造函数，初始化连接状态并注册读/关回调
 * @param scheduler 异步任务调度器
 * @param socket 已建立的 TCP 连接描述符
 */
SigConnection::SigConnection(TaskScheduler *scheduler, int socket)
    :TcpConnection(scheduler,socket)
    ,state_(NONE)
{
    // 设置数据读取回调
    this->SetReadCallback([this](std::shared_ptr<TcpConnection> conn, BufferReader& buffer){
        return this->OnRead(buffer);
    });

    // 设置连接关闭回调
    this->SetCloseCallback([this](std::shared_ptr<TcpConnection> conn){
        this->DisConnected();
    });
}

/**
 * @brief 析构函数，清理关联资源
 */
SigConnection::~SigConnection()
{
    Clear();
}

/**
 * @brief 处理客户端断开时的清理逻辑，通知关联端并释放自身注册
 */
void SigConnection::DisConnected()
{
    printf("disConnect\n");
    Clear();
}

/**
 * @brief 将指定客户端 ID 添加到关联列表，用于后续控制/转发
 * @param code 对方客户端唯一标识
 */
void SigConnection::AddCustom(const std::string &code)
{
    // 如果已存在于关联列表中，则不重复添加
    if(std::find(objectes_.begin(), objectes_.end(), code) != objectes_.end())
        return;
    objectes_.push_back(code);
}

/**
 * @brief 从关联列表移除指定客户端，并在无关联时恢复空闲状态
 * @param code 对方客户端唯一标识
 */
void SigConnection::RmoveCustom(const std::string &code)
{
    objectes_.erase(std::remove(objectes_.begin(), objectes_.end(), code), objectes_.end());
    if(objectes_.empty())
    {
        state_ = IDLE;
    }
}

/**
 * @brief 数据到达回调，循环解析并处理所有完整包
 * @param buffer 网络缓冲
 * @return 是否保持连接
 */
bool SigConnection::OnRead(BufferReader &buffer)
{
    while(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

/**
 * @brief 解析单条信令消息并根据命令类型分发处理
 * @param buffer 网络缓冲
 */
void SigConnection::HandleMessage(BufferReader &buffer)
{
    if(buffer.ReadableBytes() < sizeof(packet_head))
        return; // 包头不完整

    packet_head* data = reinterpret_cast<packet_head*>(buffer.Peek());
    if(buffer.ReadableBytes() < data->len)
        return; // 整体包未接收完

    switch (data->cmd)
    {
    case JOIN:
        HandleJion(data);// 处理加入房间或创建房间请求
        break;
    case OBTAINSTREAM:
        HandleObtainStream(data); // 处理获取流信息请求
        break;
    case CREATESTREAM:
        HandleCreateStream(data); // 处理创建流请求
        break;
    case DELETESTREAM:
        HandleDeleteStream(data); // 处理删除流请求
        break;
    case MOUSE:
    case MOUSEMOVE:
    case KEY:
    case WHEEL:
        HandleOtherMessage(data); // 处理鼠标/键盘等控制消息
        break;
    default:
        break;
    }

    // 从缓冲区移除已处理数据
    buffer.Retrieve(data->len);
}

/**
 * @brief 清理当前连接状态，将自身从 ConnectionManager 中移除，
 *        并通知所有关联客户端删除流更新其状态
 */
void SigConnection::Clear()
{
    state_ = CLOSE;
    conn_ = nullptr;

    DeleteStream_body body;
    // 通知所有关联客户端更新流数量
    for(auto &id : objectes_)
    {
        auto con = ConnectionManager::GetInstance()->QueryConn(id); // 查询关联的连接
        if(con)
        {
            auto sigCon = std::dynamic_pointer_cast<SigConnection>(con);// 转换为 SigConnection 类型
            if(sigCon)
            {
                sigCon->RmoveCustom(code_);// 从关联列表中移除当前连接
                body.SetStreamCount(sigCon->objectes_.size());// 更新流数量
                con->Send(reinterpret_cast<const char*>(&body), body.len);// 发送删除流请求
            }
        }
    }
    objectes_.clear();
    ConnectionManager::GetInstance()->RmvConn(code_);
    printf("con size: %d\n", ConnectionManager::GetInstance()->Size());
}

/** @brief 处理 JOIN 命令，完成房间加入或创建逻辑 */
void SigConnection::HandleJion(const packet_head *data)
{
    JoinReply_body reply;
    auto req = reinterpret_cast<const Join_body*>(data);
    if(IsNoJion())// 如果当前状态是 NONE，表示未加入任何房间
    {
        std::string id = req->GetId();// 获取请求中的房间 ID
        auto existing = ConnectionManager::GetInstance()->QueryConn(id);// 查询是否已有同名房间存在
        if(existing)
        {
            reply.SetCode(ERROR);// 如果房间已存在，返回错误
            Send(reinterpret_cast<const char*>(&reply), reply.len);
            return;
        }
        code_ = id;
        state_ = IDLE;
        ConnectionManager::GetInstance()->AddConn(code_, shared_from_this());// 将当前连接注册到管理器中
        printf("Join count: %d\n", ConnectionManager::GetInstance()->Size());
        reply.SetCode(SUCCESSFUL);
    }
    else
    {
        reply.SetCode(ERROR);
    }
    Send(reinterpret_cast<const char*>(&reply), reply.len);
}

/** @brief 处理 OBTAINSTREAM 命令，委派至 DoObtainStream */
void SigConnection::HandleObtainStream(const packet_head *data)
{
    DoObtainStream(data);
}

/** @brief 处理 CREATESTREAM 命令，委派至 DoCreateStream */
void SigConnection::HandleCreateStream(const packet_head *data)
{
    DoCreateStream(data);
}

/** @brief 处理 DELETESTREAM 命令，若忙碌则清理状态 */
void SigConnection::HandleDeleteStream(const packet_head *data)
{
    if(IsBusy()) Clear();
}

/**
 * @brief 处理鼠标/键盘等控制消息，转发给关联的推流端
 * @param data 包头及包体
 */
void SigConnection::HandleOtherMessage(const packet_head *data)
{
    if(conn_ && state_ == PULLER)
    {
        conn_->Send(reinterpret_cast<const char*>(data), data->len);
    }
}

/**
 * @brief 执行获取流逻辑，根据目标连接状态决定推流或拉流操作
 * @param data 实际为 ObtainStream_body 包体
 */
void SigConnection::DoObtainStream(const packet_head *data)
{
    ObtainStreamReply_body reply;
    CreateStream_body create_reply;
    std::string code = ((ObtainStream_body*)data)->GetId();
    TcpConnection::Ptr conn = ConnectionManager::GetInstance()->QueryConn(code);// 查询目标连接
    if(!conn)
    {
        printf("远程目标不存在\n");
        reply.SetCode(ERROR);
        this->Send((const char*)&reply,reply.len);
        return;
    }
    if(conn == shared_from_this()) //不能控制自己, shared_from_this() 返回当前对象的智能指针
    {
        printf("不能控制自己\n");
        reply.SetCode(ERROR);
        this->Send((const char*)&reply,reply.len);
        return; 
    }
    if(this->IsIdle())//本身是空闲就去获取流
    {
        auto con = std::dynamic_pointer_cast<SigConnection>(conn);
        switch (con->GetRoleState())//获取目标连接的状态
        {
        case IDLE://目标是空闲，我们就去通知他去推流
            printf("目标空闲\n");
            this->state_ = PULLER;
            this->AddCustom(code); //添加被控端
            con->AddCustom(code_);//被控端需要添加控制端,即互相添加新连接
            reply.SetCode(SUCCESSFUL);
            conn_ = conn; //我们就可以通过这个目标(被控端)连接器来转发消息
            //通知被控端来创建流
            con->Send((const char*)&create_reply,create_reply.len);
            this->Send((const char*)&reply,reply.len);
            break;
        case NONE:
            printf("目标没上线\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&reply,reply.len);
            break;
        case CLOSE:
            printf("目标离线\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&reply,reply.len);
            break;
        case PULLER: //目标拉流(控制端) 控制端不能控制控制端
            printf("目标忙碌\n");
            reply.SetCode(ERROR);
            this->Send((const char*)&reply,reply.len);
            break;
        case PUSHER: //推流说明他是被控端，所以我们可以去拉流
            if(con->GetStreamAddres().empty()) //异常
            {
                printf("目标正在推流，但是流地址异常\n");
                reply.SetCode(ERROR);
                this->Send((const char*)&reply,reply.len);
            }
            else//在推流，而且流地址正常
            {
                printf("目标正在推流\n");
                this->state_ = PULLER;
                this->AddCustom(code);
                con->AddCustom(code_); //互相添加新连接
                //在推流 流已经存在，就不需要重新创建流，我们只需要播放流；
                PlayStream_body play_body;
                play_body.SetCode(SUCCESSFUL);
                play_body.SetstreamAddres(con->GetStreamAddres());//设置流地址
                this->Send((const char*)&play_body,play_body.len);
            }
            break;
        default:
            break;
        }
    }
    else //本身是忙碌
    {
        //返回错误
        reply.SetCode(ERROR);
        this->Send((const char*)&reply,reply.len);
    }
}

/**
 * @brief 推流端执行创建流逻辑，通知所有拉流端开始播放或报告错误
 * @param data 实际为 CreateStreamReply_body 包体
 */
void SigConnection::DoCreateStream(const packet_head *data)
{
    PlayStream_body body;
    CreateStreamReply_body* reply = (CreateStreamReply_body*)data;
    printf("body size: %d,streamaddr: %s\n",reply->len,reply->GetstreamAddres().c_str());
    streamAddres_ = reply->GetstreamAddres();//获取流地址
    //判断所有连接的状态 ,如果连接器状态十空闲，我们就去回应
    for(auto idefy : objectes_)
    {
        TcpConnection::Ptr conn = ConnectionManager::GetInstance()->QueryConn(idefy);
        if(!conn)
        {
            this->RmoveCustom(idefy);//如果查询不到连接器，说明对方已经离线了，我们就从关联列表中移除
            continue;
        }
        auto con = std::dynamic_pointer_cast<SigConnection>(conn);
        if(streamAddres_.empty())//流地址异常
        {
            printf("流地址异常\n");
            con->state_ = IDLE;
            body.SetCode(ERROR);
            con->Send((const char*)&body,body.len);
            continue;
        }
        switch (con->GetRoleState())//获取对方连接器的状态
        {
        case NONE:
        case IDLE:
        case CLOSE:
        case PUSHER://对方正在推流，我们不能控制推流端
            body.SetCode(ERROR);
            this->RmoveCustom(con->GetCode());//从关联列表中移除
            con->Send((const char*)&body,body.len);
            break;
        case PULLER: //对方正在拉流，我们可以去推流
        this->state_ = PUSHER;
        body.SetCode(SUCCESSFUL);
        body.SetstreamAddres(streamAddres_);// 设置流地址
        printf("streamadder: %s\n",streamAddres_.c_str());
        conn->Send((const char*)&body,body.len);
        break;
        default:
            break;
        }
    }
}
