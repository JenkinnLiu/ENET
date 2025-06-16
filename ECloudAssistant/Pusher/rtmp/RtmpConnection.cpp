#include "RtmpConnection.h"
#include "rtmp.h"
#include "RtmpPublisher.h"
#include <QDebug>

RtmpConnection::RtmpConnection(std::shared_ptr<RtmpPublisher> publisher, TaskScheduler* scheduler, int sockfd)
    :RtmpConnection(scheduler,sockfd,publisher.get())
{
    handshake_.reset(new RtmpHandshake(RtmpHandshake::HANDSHAKE_S0S1S2)); //推流器应该去处理S0S1S2
    rtmp_publisher_ = publisher;
}

RtmpConnection::RtmpConnection(TaskScheduler *scheduler, int sockfd, Rtmp* rtmp)
    :TcpConnection(scheduler,sockfd)
    ,rtmp_chunk_(new RtmpChunk())
    ,state_(HANDSHAKE)
{
    amf_decoder_.reset(new AmfDecoder());
    amf_encoder_.reset(new AmfEncoder());

    max_chunk_size_ = rtmp->GetChunkSize();
    stream_path_ = rtmp->GetStreamPath();
    stream_name_ = rtmp->GetStreamName();
    app_ = rtmp->GetApp();

    //设置回调函数，处理读数据
    this->SetReadCallback([this](std::shared_ptr<TcpConnection> conn,BufferReader& buffer){
        return this->OnRead(buffer);
    });

    //设置关闭回调，释放资源
    this->SetCloseCallback([this](std::shared_ptr<TcpConnection> conn){
        this->OnClose();
    });
}

RtmpConnection::~RtmpConnection()
{
}

bool RtmpConnection::OnRead(BufferReader &buffer)
{
    bool ret = true;
    if(handshake_->IsCompleted())//是否握手完成，完成之后才能发送message
    {
        ret = HandleChunk(buffer);
    }
    else
    {
        std::shared_ptr<char> res(new char[4096],std::default_delete<char[]>());
        int res_size = handshake_->Parse(buffer,res.get(),4096);
        if(res_size < 0)
        {
            ret = false;
        }
        if(res_size > 0)
        {
            this->Send(res.get(),res_size);
        }
        if(handshake_->IsCompleted())
        {
            if(buffer.ReadableBytes() > 0)
            {
                ret = HandleChunk(buffer);
            }
            //握手完成之后，我们需要发生连接请求
            //设置块大小
            SetChunkSize();
            //发送连接请求
            Connect();
        }
    }
    return ret;
}

void RtmpConnection::OnClose()
{
    this->DeleteStream();
}

bool RtmpConnection::HandleChunk(BufferReader &buffer)
{
    //处理块
    int ret = -1;
    do
    {
        RtmpMessage rtmp_msg;
        ret = rtmp_chunk_->Parse(buffer,rtmp_msg);
        if(ret >= 0) //解析成功，需要处理message
        {
            if(rtmp_msg.IsCompleted())
            {
                if(!HandleMessage(rtmp_msg))
                {
                    return false;
                }
            }
            if(ret == 0) //缓存区没有数据
            {
                break;
            }
        }
        else
        {
            return false;
        }
    } while (buffer.ReadableBytes() > 0);
    return true;
}

bool RtmpConnection::HandleMessage(RtmpMessage &rtmp_msg) //INVOKE CHUNK_SIZE
{
    bool ret = true;
    switch (rtmp_msg.type_id)
    {
    case RTMP_INVOKE: //视频数据
        ret = HandleInvoke(rtmp_msg);
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
    //处理消息 ，需要处理amf编码数据
    bool ret = true;

    amf_decoder_->reset(); //清空解码器

    int bytes_used = amf_decoder_->decode((const char*)rtmp_msg.playload.get(),rtmp_msg.lenght,1);
    if(bytes_used < 0)
    {
        return false;
    }

    //解码成功，获取方法
    std::string method = amf_decoder_->getString();

    //继续解码
    bytes_used = amf_decoder_->decode(rtmp_msg.playload.get() + bytes_used,rtmp_msg.lenght - bytes_used);
    if(method == "_result")
    {
        ret = HandleResult(rtmp_msg);
    }
    else if(method == "onStatus")
    {
        ret = HandleOnStatus(rtmp_msg);
    }
    return ret;

}

bool RtmpConnection::Handshake()
{
    //握手 推流器 发送C0C1
    uint32_t size = 1 + 1536;
    std::shared_ptr<char> req(new char[size],std::default_delete<char[]>());
    //创建C0C1
    handshake_->BuildC0C1(req.get(),size);
    this->Send(req.get(),size);
    return true;
}

bool RtmpConnection::Connect()
{
    //创建连接
    //准备AMF编码数据
    AmfObjects objects;
    //清空编码器
    amf_encoder_->reset();

    //编码字符
    amf_encoder_->encodeString("connect",7);
    amf_encoder_->encodeNumber((double)++number_);
    objects["app"] = AmfObject(app_);
    objects["type"] = AmfObject(std::string("nonprivate"));

    //再将对象编码进去
    amf_encoder_->encodeObjects(objects);
    //更新状态
    state_ = START_CONNECT;
    SendInvokeMsg(RTMP_CHUNK_INVOKE_ID,amf_encoder_->data(),amf_encoder_->size());
    qDebug() << "Connect";
    return true;
}

bool RtmpConnection::CretaeStream()
{
    //编码AMF数据
    AmfObjects objects;
    //清空这个编码器
    amf_encoder_->reset();
    amf_encoder_->encodeString("createStream",12);
    amf_encoder_->encodeNumber((double)++number_);
    amf_encoder_->encodeObjects(objects);
    //更新状态
    state_ = START_CREATE_STREAM;
    SendInvokeMsg(RTMP_CHUNK_INVOKE_ID,amf_encoder_->data(),amf_encoder_->size());
    qDebug() << "CretaeStream";
    return true;
}

bool RtmpConnection::Publish()
{
    qDebug() << "publis";
    //编码amf
    AmfObjects objects;
    amf_encoder_->reset();

    amf_encoder_->encodeString("publish",7);
    amf_encoder_->encodeNumber((double)++number_);
    amf_encoder_->encodeObjects(objects);
    //还需要编码流地址
    amf_encoder_->encodeString(stream_name_.c_str(),(int)stream_name_.size());

    //更新状态
    state_ = START_PUBLISH;
    SendInvokeMsg(RTMP_CHUNK_INVOKE_ID,amf_encoder_->data(),amf_encoder_->size());
    return true;
}

bool RtmpConnection::DeleteStream()
{
    //编码amf
    AmfObjects objects;
    amf_encoder_->reset();

    amf_encoder_->encodeString("DeleteStream",12);
    amf_encoder_->encodeNumber((double)++number_);
    amf_encoder_->encodeObjects(objects);
    amf_encoder_->encodeNumber(stream_id_);

    //更新状态
    state_ = START_DELETE_STREAM;
    SendInvokeMsg(RTMP_CHUNK_INVOKE_ID,amf_encoder_->data(),amf_encoder_->size());
    return true;
}

bool RtmpConnection::HandleResult(RtmpMessage &rtmp_msg)
{
    //处理结果 连接和创建流，服务器会返回这个result
    bool ret = true;
    qDebug() << "HandleResult";
    if(state_ == START_CONNECT)
    {
        if(amf_decoder_->hasObject("code"))
        {
            AmfObject amfObj = amf_decoder_->getObject("code");
            if(amfObj.amf_string == "NetConnection.Connect.Success")
            {
                qDebug() << "START_CONNECT";
                CretaeStream();
                ret = true;
            }
        }
    }
    else if(state_ == START_CREATE_STREAM)
    {
        if(amf_decoder_->getNumber() > 0)
        {
            //更新流ID
            stream_id_ = (int)amf_decoder_->getNumber();
            //创建流成功，开始推流
            qDebug() << "START_CREATE_STREAM";
            this->Publish();
            ret = true;
        }
    }
    return ret;
}

bool RtmpConnection::HandleOnStatus(RtmpMessage &rtmp_msg)
{
    //onstatus 推流/删除流会得到服务器应答onStatus
    bool ret = true;

    if(state_ == START_PUBLISH)
    {
        if(amf_decoder_->hasObject("code"))
        {
            AmfObject amfObj = amf_decoder_->getObject("code");
            std::string status = amfObj.amf_string;
            if(status == "NetStream.Publish.Start")
            {
                //更新当前推流状态
                is_publishing_ = true;
            }
            else
            {
                ret = false;
            }
        }
    }
    else if(state_ == START_DELETE_STREAM)
    {
        if(amf_decoder_->hasObject("code"))
        {
            AmfObject amfObj = amf_decoder_->getObject("code");
            if(amfObj.amf_string != "NetStream.Unpublish.Success")//取消推流失败
            {
                ret = false;
            }
        }
    }
    return ret;
}

bool RtmpConnection::SendVideoData(uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size)
{
    //发送视频数据
    if(playload_size <= 0)
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_VIDEO;
    rtmp_msg._timestamp = timestamp;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;
    //发送
    SendRtmpChunks(RTMP_CHUNK_VIDEO_ID,rtmp_msg);
    return true;
}

bool RtmpConnection::SendAudioData(uint64_t timestamp, std::shared_ptr<char> playload, uint32_t playload_size)
{
    //发送音频数据
    if(playload_size <= 0)
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_AUDIO;
    rtmp_msg._timestamp = timestamp;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;
    //发送
    SendRtmpChunks(RTMP_CHUNK_AUDIO_ID,rtmp_msg);
    return true;
}

void RtmpConnection::SetChunkSize()
{
    rtmp_chunk_->SetOutChunkSize(max_chunk_size_);
    std::shared_ptr<char> data(new char[4],std::default_delete<char[]>());
    WriteUint32BE(data.get(),max_chunk_size_);
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_SET_CHUNK_SIZE;
    rtmp_msg.playload = data;
    rtmp_msg.lenght = 4;
    SendRtmpChunks(RTMP_CHUNK_CONTROL_ID,rtmp_msg);
}

bool RtmpConnection::SendInvokeMsg(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
{
    if(this->IsClosed())
    {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_INVOKE;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = stream_id_;
    rtmp_msg.playload = playload;
    rtmp_msg.lenght = playload_size;
    SendRtmpChunks(csid,rtmp_msg);
    return true;
}

bool RtmpConnection::SendNotifyMsg(uint32_t csid, std::shared_ptr<char> playload, uint32_t playload_size)
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

bool RtmpConnection::IsKeyFrame(std::shared_ptr<char> data, uint32_t size)
{
    uint8_t frame_type = (data.get()[0] >> 4) & 0x0f;
    uint8_t code_id = data.get()[0] & 0x0f;
    return (frame_type == 1 && code_id == RTMP_CODEC_ID_H264);
}
void RtmpConnection::SendRtmpChunks(uint32_t csid, RtmpMessage &rtmp_msg)
{
    uint32_t capacity = rtmp_msg.lenght + rtmp_msg.lenght / max_chunk_size_ * 5 + 1024; 
    std::shared_ptr<char> buffer(new char[capacity],std::default_delete<char[]>());
    int size = rtmp_chunk_->CreateChunk(csid,rtmp_msg,buffer.get(),capacity);
    if(size > 0)
    {
        this->Send(buffer.get(),size);
    }
}


