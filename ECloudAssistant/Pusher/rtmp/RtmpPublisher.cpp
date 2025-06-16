#include "RtmpPublisher.h"

std::shared_ptr<RtmpPublisher> RtmpPublisher::Create(EventLoop *loop)
{
    std::shared_ptr<RtmpPublisher> publisher(new RtmpPublisher(loop));
    return publisher;
}

RtmpPublisher::RtmpPublisher(EventLoop *event_loop)
    :event_loop_(event_loop)
{

}

RtmpPublisher::~RtmpPublisher()
{

}

int RtmpPublisher::SetMediaInfo(MediaInfo media_info)
{
    //设置音视频消息
    media_info_ = media_info;

    if(media_info_.audio_codec_id == RTMP_CODEC_ID_AAC)
    {
        if(media_info_.audio_specific_config_size > 0)
        {
            //更新这个编码参数大小，我们需要添加两个字节
            aac_sequence_header_size_ = media_info_.audio_specific_config_size + 2;
            aac_sequence_header_.reset(new char[aac_sequence_header_size_]);
            //填充字段
            uint8_t* data = (uint8_t*)aac_sequence_header_.get();
            //audio tag
            data[0] = 0xAF;
            data[1] = 0;

            //拷贝数据
            memcpy(data + 2,media_info_.audio_specific_config.get(),media_info_.audio_specific_config_size);
        }
        else
        {
            media_info_.audio_codec_id = 0;
        }
    }

    if(media_info_.video_codec_id == RTMP_CODEC_ID_H264)
    {
        if(media_info_.sps_size > 0 && media_info_.pps_size > 0)
        {
            //申请内存空间
            avc_sequence_header_.reset(new char[4096],std::default_delete<char[]>());
            //填充字段
            uint8_t* data = (uint8_t*)avc_sequence_header_.get();
            uint32_t index = 0;

            data[index++] = 0x17; //keyframe
            data[index++] = 0; //avc header

            data[index++] = 0;
            data[index++] = 0;
            data[index++] = 0;

            data[index++] = 0x01;
            data[index++] = media_info_.sps.get()[1];
            data[index++] = media_info_.sps.get()[2];
            data[index++] = media_info_.sps.get()[3];
            data[index++] = 0xff;

            //sps nums;
            data[index++] = 0xE1;

            //spa data lenght;
            data[index++] = media_info_.sps_size >> 8;
            data[index++] = media_info_.sps_size & 0xff;

            //sps data
            memcpy(data + index,media_info_.sps.get(),media_info_.sps_size);
            //更新索引
            index += media_info_.sps_size;

            //pps
            data[index++] = 0x01;
            //pps lenght
            data[index++] = media_info_.pps_size >> 8;
            data[index++] = media_info_.pps_size & 0xff;

            //sps data
            memcpy(data + index,media_info_.pps.get(),media_info_.pps_size);
            index += media_info_.pps_size;

            //更新这个头部信息总长度
            avc_sequence_header_size_ = index;
        }
    }
    return 0;
}

int RtmpPublisher::OpenUrl(std::string url, int msec)
{
    //打开url
    if(ParseRtmpUrl(url) != 0)
    {
        return -1;
    }

    if(rtmp_conn_) //连接存在，我们就需要断开连接
    {
        std::shared_ptr<RtmpConnection> con = rtmp_conn_;
        rtmp_conn_ = nullptr;
        con->DisConnect();
    }

    //创建连接
    TcpSocket socket;
    socket.Create();
    if(!socket.Connect(ip_,port_))
    {
        socket.Close();
        return -1;
    }

    //创建rtmp_conn_
    rtmp_conn_.reset(new RtmpConnection(shared_from_this(),event_loop_->GetTaskSchduler().get(),socket.GetSocket()));

    //连接成功，我们需要开始rtmp握手
    rtmp_conn_->Handshake();
    return 0;
}

int RtmpPublisher::PushVideoFrame(uint8_t *data, uint32_t size)
{
    if(rtmp_conn_ == nullptr || rtmp_conn_->IsClosed() || size <= 5)
    {
        return -1;
    }

    if(media_info_.video_codec_id == RTMP_CODEC_ID_H264)
    {
        //是否已经发送第一个包
        if(!has_key_frame_)
            {
            if(this->IsKeyFrame(data,size))
            {
                has_key_frame_ = true;
                rtmp_conn_->SendVideoData(0,avc_sequence_header_,avc_sequence_header_size_);
                rtmp_conn_->SendVideoData(0,aac_sequence_header_,aac_sequence_header_size_);
            }
            else
            {
                return 0;
            }
        }
    }
    uint64_t timestamp = timestamp_.Elapsed();
    //如果已经发送第一个包
    //发送视频 tag + 264数据
    std::shared_ptr<char> playload(new char[size + 4096],std::default_delete<char[]>());
    uint32_t playload_size = 0;

    //填充这个tag
    uint8_t* body = (uint8_t*)playload.get();
    uint32_t index = 0;
    body[index++] = this->IsKeyFrame(data,size) ? 0x17 : 0x27;
    body[index++] = 1;

    body[index++] = 0;
    body[index++] = 0;
    body[index++] = 0;

    body[index++] = (size >> 24) & 0xff;
    body[index++] = (size >> 16) & 0xff;
    body[index++] = (size >> 8) & 0xff;
    body[index++] = (size) & 0xff;

    //拷贝
    memcpy(body + index ,data,size);
    index += size;
    playload_size = index;
    rtmp_conn_->SendVideoData(timestamp,playload,playload_size);

}

int RtmpPublisher::PushAudioFrame(uint8_t *data, uint32_t size)
{
    if(rtmp_conn_ == nullptr || rtmp_conn_->IsClosed() || size <= 0)
    {
        return -1;
    }
    if(media_info_.audio_codec_id == RTMP_CODEC_ID_AAC && has_key_frame_)
    {
        uint64_t timestamp = timestamp_.Elapsed();
        uint32_t play_size = size + 2;
        std::shared_ptr<char> playload(new char[size + 2],std::default_delete<char[]>());
        //填充字段
        playload.get()[0] = 0xAF;
        playload.get()[1] = 1;
        memcpy(playload.get() + 2 ,data,size);
        rtmp_conn_->SendAudioData(timestamp,playload,play_size);
    }
    return 0;
}

void RtmpPublisher::Close()
{
    if(rtmp_conn_)
    {
        std::shared_ptr<RtmpConnection> con = rtmp_conn_;
        rtmp_conn_ = nullptr;
        con->DisConnect();
        has_key_frame_ = false;
    }

}

bool RtmpPublisher::IsConnected()
{
    if(rtmp_conn_)
    {
        return (!rtmp_conn_->IsClosed());
    }
    return false;
}

bool RtmpPublisher::IsKeyFrame(uint8_t *data, uint32_t size)
{
    //判断关键帧 startcode 3 4
    int startcode = 0;
    if(data[0] == 0 && data[1] == 0 && data[2] == 0)
    {
        startcode = 3;
    }
    else if(data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0)
    {
        startcode = 4;
    }

    //再去获取类型
    int type = data[startcode] & 0x1f;
    if(type == 5 || type == 7)//关键帧
    {
        return true;
    }
    return false;
}

