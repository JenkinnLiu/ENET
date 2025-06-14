/**
 * @file RtmpConnection.cpp
 * @brief 实现 RtmpConnection 类，用于处理单个连接的 RTMP 协议全流程。
 *
 * - 握手（C0/C1/C2, S0/S1/S2）
 * - Chunk 接收与组装
 * - AMF0 命令消息解码/编码
 * - connect/createStream/publish/play/deleteStream 等命令处理
 * - 音视频帧的序列头缓存与分发
 */
#include "RtmpConnection.h"
#include "rtmp.h"
#include "RtmpServer.h"

/**
 * @brief 构造函数（服务端接口），设置握手状态并保存服务器引用
 */
RtmpConnection::RtmpConnection(std::shared_ptr<RtmpServer> rtmp_server, TaskScheduler *scheduler, int socket)
    :RtmpConnection(scheduler,socket,rtmp_server.get())
{
    handshake_.reset(new RtmpHandshake(RtmpHandshake::HANDSHAKE_C0C1));  // 初始化握手状态
    rtmp_server_ = rtmp_server;                                         // 保存服务端实例
}

/**
 * @brief 私有构造，初始化 TcpConnection、Chunk 解析器和默认状态
 */
RtmpConnection::RtmpConnection(TaskScheduler* scheduler,int socket,Rtmp* rtmp)
    :TcpConnection(scheduler,socket)
    ,rtmp_chunk_(new RtmpChunk())
    ,state_(HANDSHAKE)
{
    // 初始化带宽、chunk 大小以及流路径信息
    peer_width_ = rtmp->GetPeerBandwidth();    
    ackonwledgement_size_ = rtmp->GetAcknowledgementSize();
    max_chunk_size_ = rtmp->GetChunkSize();
    stream_path_ = rtmp->GetStreamPath();
    stream_name_ = rtmp->GetStreamName();
    app_ = rtmp->GetApp();

    // 设置网络回调：接收到数据调用 OnRead
    this->SetReadCallback([this](std::shared_ptr<TcpConnection> conn,BufferReader& buffer){
        return this->OnRead(buffer);
    });
    // 设置关闭回调：连接关闭时清理流
    this->SetCloseCallback([this](std::shared_ptr<TcpConnection> conn){
        this->OnClose();
    });
}

/**
 * @brief 析构函数，负责清理资源
 */
RtmpConnection::~RtmpConnection()
{
}

/**
 * @brief 数据到达回调：首先完成握手，然后按 Chunk 解析并处理消息
 * @param buffer 输入缓冲区
 * @return true 表示处理成功，可继续接收，false 表示出错需断开
 */
bool RtmpConnection::OnRead(BufferReader &buffer)
{
    bool ret = true;
    // 如果握手已完成，则进入 Chunk 层继续解析和处理消息
    if (handshake_->IsCompleted())
    {
        ret = HandleChunk(buffer);
    }
    else
    {
        // 握手未完成：解析客户端发送的握手包 C0/C1
        std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
        int res_size = handshake_->Parse(buffer, res.get(), 4096);
        if (res_size < 0)
        {
            // 握手失败，直接断开连接
            return false;
        }
        if (res_size > 0)
        {
            // 发送服务端握手响应 S0/S1/S2
            this->Send(res.get(), res_size);
        }
        // 如果握手在本次交互中完成，且缓存中还有剩余数据，则继续处理剩余 Chunk
        if (handshake_->IsCompleted() && buffer.ReadableBytes() > 0)
        {
            ret = HandleChunk(buffer);
        }
    }
    return ret;
}

/**
 * @brief 连接关闭回调：在连接关闭时优雅地删除当前流，释放会话资源
 */
void RtmpConnection::OnClose()
{
    this->HandleDeleteStream();
}

/**
 * @brief 解析并组装接收到的 Chunk 数据，直到缓冲区耗尽或发生错误
 * @param buffer 输入缓冲区
 * @return true 解析及处理成功，false 表示发生错误需断开
 */
bool RtmpConnection::HandleChunk(BufferReader &buffer)
{
    int ret = -1;
    // 循环读取并解析 Chunk，直到缓冲区空或发生错误
    do
    {
        RtmpMessage rtmp_msg;
        ret = rtmp_chunk_->Parse(buffer, rtmp_msg);// 解析 Chunk 数据
        if (ret < 0)
            return false;               // Chunk 解析错误，需断开
        if (ret >= 0 && rtmp_msg.IsCompleted())
        {
            // 完整消息到达，进入上层业务处理
            if (!HandleMessage(rtmp_msg))
                return false;           // 消息处理失败，断开
        }
    } while (buffer.ReadableBytes() > 0 && ret > 0);
    return true;
}

/**
 * @brief 处理完整的 RTMP 消息，根据消息类型分发到对应处理函数
 * @param rtmp_msg 完整的 RTMP 消息结构
 * @return true 处理成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandleMessage(RtmpMessage &rtmp_msg)
{
    bool ret = true;
    switch (rtmp_msg.type_id)
    {
    case RTMP_VIDEO: //视频数据
        ret = HandleVideo(rtmp_msg);
        break;
    case RTMP_AUDIO:
        ret = HandleAudio(rtmp_msg);
        break;
    case RTMP_INVOKE:
        ret = HandleInvoke(rtmp_msg);// 处理启动命令消息
        break;
    case RTMP_NOTIFY:
        ret = HandleNotify(rtmp_msg);// 处理通知数据消息
        break;
    case RTMP_SET_CHUNK_SIZE:
        rtmp_chunk_->SetInChunkSize(ReadUint32BE(rtmp_msg.playload.get()));
        break;
    default:
        break;
    }
    return ret;
}

bool RtmpConnection::HandleInvoke(RtmpMessage &rtmp_msg)
{
    // 重置 AMF 解码器以解析新消息
    amf_decoder_.reset();
    // 首先解析方法名（第一个 AMF0 字符串）
    int bytes_used = amf_decoder_.decode(rtmp_msg.playload.get(), rtmp_msg.lenght, 1);
    if (bytes_used < 0)
    {
        printf("AMF 解码失败\n");
        return false;
    }
    std::string method = amf_decoder_.getString();
    // 根据 stream_id 判断是 connect/createStream（stream_id=0）还是 publish/play/DeleteStream
    if (rtmp_msg.stream_id == 0)
    {
        // 解析剩余参数，处理连接或创建流
        bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.lenght - bytes_used);
        if (method == "connect")
            return HandleConnect();// 处理 connect 命令，建立连接
        if (method == "createStream")
            return HandleCreateStream(); // 处理 createStream 命令，创建流
    }
    else if (rtmp_msg.stream_id == stream_id_)// 如果是已分配的 stream_id
    {
        // 对于已分配的 stream_id，继续解析参数（通常包含流名称）,这里解析3是因为第一个参数是方法名
        // 解析方法名后，继续解析剩余参数
        bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.lenght - bytes_used, 3);
        stream_name_ = amf_decoder_.getString();
        stream_path_ = "/" + app_ + "/" + stream_name_;
        // 如果还有剩余参数，继续解析
        if (rtmp_msg.lenght > bytes_used)
            bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used, rtmp_msg.lenght - bytes_used);
        // 分发到对应业务处理
        if (method == "publish")
            return HandlePublish();
        if (method == "play")
            return HandlePlay();
        if (method == "DeleteStream")
            return HandleDeleteStream();
    }
    return true;
}

/**
 * @brief 处理通知消息，解析元数据并发送给对应的 RtmpSession
 * @param rtmp_msg 完整的 RTMP 消息
 * @return true 处理成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandleNotify(RtmpMessage &rtmp_msg)
{
    //准备解码器
    amf_decoder_.reset();

    //解析消息名称
    int bytes_used = amf_decoder_.decode(rtmp_msg.playload.get(),rtmp_msg.lenght,1);

    std::string method = amf_decoder_.getString();
    if(method == "@setDataFrame")// 如果是元数据通知
    {
        amf_decoder_.reset();
        //继续解析剩余的参数
        bytes_used = amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used,rtmp_msg.lenght - bytes_used,1);
        if(bytes_used < 0)
        {
            return false;
        }
        //是不是元数据
        if(amf_decoder_.getString() == "onMetaData")// 如果是元数据
        {
            //解析剩余的元数据
            bytes_used += amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used,rtmp_msg.lenght - bytes_used);
        }
        else
        {
            //不是元数据，直接返回
            return true;
        }
        {
            // 解析元数据对象
            amf_decoder_.decode(rtmp_msg.playload.get() + bytes_used,rtmp_msg.lenght - bytes_used);
            meta_data_ = amf_decoder_.getObjects();// 获取元数据对象
        }

        //设置元数据
        //TODO 获取session设置元数据
        auto server = rtmp_server_.lock();
        if(!server)
        {
            return false;
        }

        auto session = rtmp_session_.lock();
        if(session)
        {
            session->SendMetaData(meta_data_);
        }
    }
    return true;
}

bool RtmpConnection::HandleAudio(RtmpMessage &rtmp_msg)
{
    // 提取音频格式信息：高 4 位为 sound_format，低 4 位为编码 id
    uint8_t* payload = (uint8_t*)rtmp_msg.playload.get();
    uint8_t sound_format = (payload[0] >> 4) & 0x0f;
    // 如果是 AAC 且第二字节为 0，表示这是 AAC sequence header
    if (sound_format == RTMP_CODEC_ID_AAC && payload[1] == 0)
    {
        // 缓存 AAC 序列头以便后续发送
        aac_sequence_header_szie_ = rtmp_msg.lenght;
        aac_sequence_header_.reset(new char[rtmp_msg.lenght], std::default_delete<char[]>());
        memcpy(aac_sequence_header_.get(), rtmp_msg.playload.get(), aac_sequence_header_szie_);
        // 通知 Session 设置序列头
        session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_szie_);
        // 调整消息类型为 AAC 序列头
        type = RTMP_AAC_SEQUENCE_HEADER;
    }
    // 将音频数据分发给对应 Session
    session->SendMediaData(type, rtmp_msg.timestamp, rtmp_msg.playload, rtmp_msg.lenght);
    return true;
}

bool RtmpConnection::HandleVideo(RtmpMessage &rtmp_msg)
{
    // 提取视频帧类型和编码 ID
    uint8_t* payload = (uint8_t*)rtmp_msg.playload.get();
    uint8_t frame_type = (payload[0] >> 4) & 0x0f;
    uint8_t codec_id   = payload[0] & 0x0f;
    // 如果是关键帧的 AVC 序列头（frame_type=1, codec_id=7 且 next 字节=0）
    if (frame_type == 1 && codec_id == RTMP_CODEC_ID_H264 && payload[1] == 0)
    {
        // 缓存 AVC 序列头，通知 Session
        avc_sequence_header_size_ = rtmp_msg.lenght;
        avc_sequence_header_.reset(new char[rtmp_msg.lenght], std::default_delete<char[]>());
        memcpy(avc_sequence_header_.get(), rtmp_msg.playload.get(), avc_sequence_header_size_);
        session->SetAvcSequenceHeader(avc_sequence_header_, avc_sequence_header_size_);
        type = RTMP_AVC_SEQUENCE_HEADER;
    }
    // 分发视频数据
    session->SendMediaData(type, rtmp_msg.timestamp, rtmp_msg.playload, rtmp_msg.lenght);
    return true;
}

/**
 * @brief 处理 connect 命令，建立 RTMP 连接
 * @return true 连接处理成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandleConnect()
{
    // 校验必需的 “app” 参数是否存在
    if (!amf_decoder_.hasObject("app")) 
        return false;
    // 读取并存储 app 名称
    app_ = amf_decoder_.getObject("app").amf_string;//获取应用程序的值
    if (app_.empty()) 
        return false;
    // 发送 ack、带宽、chunk 大小控制命令
    SendAcknowlegement();
    SetPeerBandWidth();
    SetChunkSize();
    // 构造 _result invoke 应答并发送给客户端
    AmfObjects objects;
    amf_encoder_.reset();
    //编码结果
    amf_encoder_.encodeString("_result",7);
    amf_encoder_.encodeNumber(amf_decoder_.getNumber());

    objects["fmsVer"] = AmfObject(std::string("FMS/4,5,0,297"));
    objects["capabilities"] = AmfObject(255.0);
    objects["mode"] = AmfObject(1.0);
    amf_encoder_.encodeObjects(objects);
    //清空对象
    objects.clear();
    //添加参数
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetConnection.Connect.Success"));
    objects["description"] = AmfObject(std::string("Connection succeeded"));
    objects["objectEncoding"] = AmfObject(0.0);
    amf_encoder_.encodeObjects(objects);
    SendInvokeMessage(RTMP_CHUNK_INVOKE_ID,amf_encoder_.data(),amf_encoder_.size());
    printf("HandleConnect\n");
    return true;
}

/**
 * @brief 处理 createStream 命令，创建一个新的流并返回 stream_id
 * @return true 流创建成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandleCreateStream()
{

    int stream_id = rtmp_chunk_->GetStreamId();

    AmfObjects objects;
    //结果
    amf_encoder_.reset();
    amf_encoder_.encodeString("_result",7);
    amf_encoder_.encodeNumber(amf_decoder_.getNumber());
    //需要填一个空对象
    amf_encoder_.encodeObjects(objects);
    amf_encoder_.encodeNumber(stream_id);
    SendInvokeMessage(RTMP_CHUNK_INVOKE_ID,amf_encoder_.data(),amf_encoder_.size());
    stream_id_ = stream_id;
    return true;
}

/**
 * @brief 处理 publish 命令，开始推流并通知客户端
 * @return true 推流处理成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandlePublish()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }

    AmfObjects objects;
    amf_encoder_.reset();
    amf_encoder_.encodeString("onStatus",8);
    amf_encoder_.encodeNumber(0);
    amf_encoder_.encodeObjects(objects);

    bool is_error = false;
    //我们需判断是否已经推流
    if(server->HasPublisher(stream_path_))
    {
        printf("HasPublisher\n");
        is_error = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));//说明当前这个流已经推送
        objects["description"] = AmfObject(std::string("Stream already publishing."));
    }
    //状态是推流状态，也不能推
    else if(state_ == START_PUBLISH)
    {
        printf("START_PUBLISH\n");
        is_error = true;
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.BadConnection"));//说明当前这个流已经推送
        objects["description"] = AmfObject(std::string("Stream already publishing."));
    }
    else
    {
        objects["level"] = AmfObject(std::string("status"));
        objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));//说明当前这个流已经推送
        objects["description"] = AmfObject(std::string("Start publishing."));

        //TODO
        //添加Session
        server->AddSesion(stream_path_);// 添加当前流的会话
        rtmp_session_ = server->GetSession(stream_path_);// 获取当前流的会话

        if(server)
        {
            server->NotifyEvent("publish.start",stream_path_);// 通知服务器有新的流开始推送
        }
    }
    amf_encoder_.encodeObjects(objects);
    SendInvokeMessage(RTMP_CHUNK_INVOKE_ID,amf_encoder_.data(),amf_encoder_.size());// 发送 invoke 消息

    if(is_error)
    {
        return false;
    }else
    {
        state_ = START_PUBLISH;
        is_publishing_ = true;
    }

    auto session = rtmp_session_.lock();
    if(session)
    {
        session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));// 添加当前连接为流的接收者
    }
    return true;
}

/**
 * @brief 处理 play 命令，开始播放流并通知客户端
 * @return true 播放处理成功，false 表示处理失败需断开连接
 */
bool RtmpConnection::HandlePlay()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }
    AmfObjects objects;
    amf_encoder_.reset();

    //添加应答
    amf_encoder_.encodeString("onStatus",8);
    amf_encoder_.encodeNumber(0);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Reset"));//强制丢弃旧的视频缓冲
    objects["description"] = AmfObject(std::string("Resetting ond playing stream."));
    amf_encoder_.encodeObjects(objects);
    if(!SendInvokeMessage(RTMP_CHUNK_INVOKE_ID,amf_encoder_.data(),amf_encoder_.size()))
    {
        return false;
    }

    //在发送play
    objects.clear();
    amf_encoder_.reset();
    amf_encoder_.encodeString("onStatus",8);
    amf_encoder_.encodeNumber(0);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Start"));
    objects["description"] = AmfObject(std::string("Started playing."));
    amf_encoder_.encodeObjects(objects);
    if(!SendInvokeMessage(RTMP_CHUNK_INVOKE_ID,amf_encoder_.data(),amf_encoder_.size()))
    {
        return false;
    }

    //再通知客户端权限
    amf_encoder_.reset();
    amf_encoder_.encodeString("|RtmpSampleAccess",17);
    amf_encoder_.encodeBoolean(true); //允许读
    amf_encoder_.encodeBoolean(true); //允许写
    if(!this->SendNotifyMessage(RTMP_CHUNK_DATA_ID,amf_encoder_.data(),amf_encoder_.size()))
    {
        return false;
    }

    //更新状态
    state_ = START_PLAY;
    //TODO 
    //session添加客户端
    rtmp_session_ = server->GetSession(stream_path_);
    auto session = rtmp_session_.lock();
    if(session)
    {
        session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));// 添加客户端
    }

    if(server)
    {
        server->NotifyEvent("Play.start",stream_path_);// 通知服务器有新的流开始播放
    }

    return true;
}

bool RtmpConnection::HandleDeleteStream()
{
    auto server = rtmp_server_.lock();
    if(!server)
    {
        return false;
    }

    if(stream_path_ != "")
    {
        //TODO session移除会话
        auto session = rtmp_session_.lock();
        if(session)
        {
            auto conn = std::dynamic_pointer_cast<RtmpSink>(shared_from_this());
            GetTaskSchduler()->AddTimer([session,conn](){
                session->RemoveSink(conn);
                return false;
            },1);
            if(is_publishing_)
            {
                server->NotifyEvent("publish,stop",stream_path_);
            }
            else if(is_playing_)
            {
                server->NotifyEvent("play.stop",stream_path_);
            }
        }

    is_playing_ = false;
    is_publishing_ = false;
    has_key_frame = false;
    rtmp_chunk_->Clear();
    }
    return true;
}

/**
 * @brief 设置对等方的带宽限制，发送 RTMP_BANDWIDTH_SIZE 消息
 */
void RtmpConnection::SetPeerBandWidth()
{
    std::shared_ptr<char> data(new char[5],std::default_delete<char[]>());// 分配 5 字节空间
    WriteUint32BE(data.get(),peer_width_);// 前 4 字节为带宽大小
    data.get()[4] = 2; // 0 1 2, 0表示严格限制带宽，1表示软限制，允许超出，2 表示根据网络状况动态调整带宽
    // 发送 RTMP_BANDWIDTH_SIZE 消息
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_BANDWIDTH_SIZE;// 设置消息类型为带宽限制
    rtmp_msg.playload = data;// 设置负载为带宽数据
    rtmp_msg.lenght = 5;// 设置负载长度为 5 字节
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID,rtmp_msg);// 发送控制消息
}

/**
 * @brief 发送确认消息，通知对端已接收数据
 */
void RtmpConnection::SendAcknowlegement()
{
    std::shared_ptr<char> data(new char[4],std::default_delete<char[]>());// 分配 4 字节空间
    // 将确认窗口大小写入数据
    WriteUint32BE(data.get(),ackonwledgement_size_);// 前 4 字节为确认窗口大小
    // 创建 RTMP 消息并设置类型和负载

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_ACK_SIZE;// 设置消息类型为确认大小
    rtmp_msg.playload = data;// 设置负载为确认数据
    rtmp_msg.lenght = 4;// 设置负载长度为 4 字节
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID,rtmp_msg);// 发送确认消息
}

/**
 * @brief 设置最大 Chunk 大小，发送 RTMP_SET_CHUNK_SIZE 消息
 */
void RtmpConnection::SetChunkSize()
{
    rtmp_chunk_->SetOutChunkSize(max_chunk_size_);// 设置输出 Chunk 大小
    // 分配 4 字节空间用于存储 Chunk 大小
    std::shared_ptr<char> data(new char[4],std::default_delete<char[]>());
    WriteUint32BE(data.get(),max_chunk_size_);
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_SET_CHUNK_SIZE;
    rtmp_msg.playload = data;
    rtmp_msg.lenght = 4;
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID,rtmp_msg);// 发送设置 Chunk 大小消息
}

/**
 * @brief 发送 invoke 消息，通常用于启动 RTMP 流
 * @param csid Chunk Stream ID，用于标识消息流
 * @param playload 消息负载数据
 * @param playload_size 负载数据大小
 * @return true 发送成功，false 表示连接已关闭或发送失败
 */
bool RtmpConnection::SendInvokeMessage(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->IsClosed())
    {
        return false;
    }

    // 创建 RTMP 消息并设置类型、时间戳、流 ID 和负载
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_INVOKE;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;
    SendRtmpChunks(csid,rtmp_msg);// 发送 RTMP 分片
    return true;
}

/**
 * @brief 发送 notify 消息，通常用于通知客户端某些事件
 * @param csid Chunk Stream ID，用于标识消息流
 * @param playload 消息负载数据
 * @param playload_size 负载数据大小
 * @return true 发送成功，false 表示连接已关闭或发送失败
 */
bool RtmpConnection::SendNotifyMessage(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->IsClosed())
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_NOTIFY;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;
    SendRtmpChunks(csid,rtmp_msg);
    return true;
}

/**
 * @brief 判断给定数据是否为 H264 关键帧
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return true 如果是关键帧，false 否则
 */
bool RtmpConnection::IsKeyFrame(std::shared_ptr<char> data, uint32_t size)
{
    uint8_t frame_type = (data.get()[0] >> 4) & 0x0f;
    uint8_t code_id = data.get()[0] & 0x0f;
    return (frame_type == 1 && code_id == RTMP_CODEC_ID_H264);// 关键帧的 frame_type 为 1，编码 ID 为 H264
}

/**
 * @brief 将 RTMP 消息分片并发送，考虑 Chunk 大小和额外的头部信息
 * @param csid Chunk Stream ID，用于标识消息流
 * @param rtmp_msg 完整的 RTMP 消息
 */
void RtmpConnection::SendRtmpChunks(uint32_t csid, RtmpMessage &rtmp_msg)
{
     // 计算缓冲区容量，考虑分片和额外空间， 具体来说, rtmp_msg.lenght 是负载长度，除以 max_chunk_size_ 得到分片数，
    // 每个分片需要额外的 5 字节头部信息（chunk header），再加上 1024 字节的预留空间
    uint32_t capacity = rtmp_msg.lenght + rtmp_msg.lenght / max_chunk_size_ * 5 + 1024;
    std::shared_ptr<char> buffer(new char[capacity],std::default_delete<char[]>());// 分配缓冲区
    int size = rtmp_chunk_->CreateChunk(csid,rtmp_msg,buffer.get(),capacity);// 创建分片数据
    if(size > 0)
    {
        this->Send(buffer.get(),size);// 发送分片数据
    }
}

/**
 * @brief 发送元数据，通常用于视频流的元信息
 * @param metaData 元数据对象集合
 * @return true 发送成功，false 表示连接已关闭或发送失败
 */
bool RtmpConnection::SendMetaData(AmfObjects metaData)
{
    if(this->IsClosed())
    {
        return false;
    }

    if(meta_data_.size() == 0)
    {
        return false;
    }

    amf_encoder_.reset();
    amf_encoder_.encodeString("onMetaData",10);
    amf_encoder_.encodeECMA(meta_data_);// 编码元数据对象集合
    // 将编码后的数据发送为 notify 消息
    if(!SendNotifyMessage(RTMP_CHUNK_DATA_ID,amf_encoder_.data(),amf_encoder_.size()))
    {
        return false;
    }
    return true;
}

/**
 * @brief 发送音视频数据，处理序列头和关键帧逻辑
 * @param type 数据类型（视频或音频）
 * @param timestamp 时间戳
 * @param playload 数据负载
 * @param playload_size 负载大小
 * @return true 发送成功，false 表示连接已关闭或发送失败
 */
bool RtmpConnection::SendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->IsClosed())
    {
        return false;
    }

    if(playload_size == 0)
    {
        return false;
    }

    is_playing_ = true;

    if(type == RTMP_AVC_SEQUENCE_HEADER)
    {
        avc_sequence_header_ = playload;
        avc_sequence_header_size_ = playload_size;
    }
    else if(type == RTMP_AAC_SEQUENCE_HEADER)
    {
        aac_sequence_header_ = playload;
        aac_sequence_header_szie_ = playload_size;
    }

    if(!has_key_frame && avc_sequence_header_size_ > 0
    && (type != RTMP_AVC_SEQUENCE_HEADER)
    && (type != RTMP_AAC_SEQUENCE_HEADER)) //说明数据包既不是序列头，还没有收到关键帧
    {
        //开始判断是否为关键帧
        // 关键帧的作用是为了让播放器能够快速定位到视频的关键位置，通常在视频流开始时发送一个关键帧
        // 如果还没有收到关键帧，则检查当前数据包是否为关键帧
        // 如果没有收到关键帧，则继续等待
        // 如果当前数据包不是关键帧，则直接返回
        if(IsKeyFrame(playload,playload_size))
        {
            has_key_frame = true;
        }
        else
        {
            return true;
        }
    }

    //收到关键帧之后开始发送message
    RtmpMessage rtmp_msg;
    rtmp_msg._timestamp = timestamp;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;

    //还有msg类型
    if(type == RTMP_VIDEO ||type ==RTMP_AVC_SEQUENCE_HEADER)
    {
        rtmp_msg.type_id = RTMP_VIDEO;
        SendRtmpChunks(RTMP_CHUNK_VIDEO_ID,rtmp_msg);
    }
    else if(type == RTMP_AUDIO ||type ==RTMP_AAC_SEQUENCE_HEADER)
    {
        rtmp_msg.type_id = RTMP_AUDIO;
        SendRtmpChunks(RTMP_CHUNK_AUDIO_ID,rtmp_msg);
    }
    return true;

}
