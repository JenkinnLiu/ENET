/**
 * @file RtmpPublisher.cpp
 * @brief RTMP 推流器实现
 *
 * 实现 RtmpPublisher 类的各方法，包括实例创建、媒体信息设置、
 * 推流地址打开、推送音视频帧、关闭连接及关键帧判断等逻辑。
 */

#include "RtmpPublisher.h"

/**
 * @brief 创建并返回 RtmpPublisher 实例
 * @param loop 事件循环指针
 * @return RtmpPublisher 共享指针
 */
std::shared_ptr<RtmpPublisher> RtmpPublisher::Create(EventLoop *loop)
{
    std::shared_ptr<RtmpPublisher> publisher(new RtmpPublisher(loop));
    return publisher;
}

/**
 * @brief 私有构造函数，初始化事件循环指针
 * @param event_loop 事件循环指针
 */
RtmpPublisher::RtmpPublisher(EventLoop *event_loop)
    :event_loop_(event_loop)
{

}

/**
 * @brief 析构函数，自动关闭并释放资源
 */
RtmpPublisher::~RtmpPublisher()
{

}

/**
 * @brief 设置媒体信息并生成 H.264/AAC 序列头
 * @param media_info 媒体信息，包含编码格式、SPS/PPS、AAC 配置等
 * @return 0 表示成功，非 0 表示失败
 */
int RtmpPublisher::SetMediaInfo(MediaInfo media_info)
{
    //设置音视频消息
    media_info_ = media_info;

    if(media_info_.audio_codec_id == RTMP_CODEC_ID_AAC)// 如果是 AAC 编码
    {
        if(media_info_.audio_specific_config_size > 0)
        {
            //更新这个编码参数大小，我们需要添加两个字节
            aac_sequence_header_size_ = media_info_.audio_specific_config_size + 2;
            aac_sequence_header_.reset(new char[aac_sequence_header_size_]);
            //填充字段
            uint8_t* data = (uint8_t*)aac_sequence_header_.get();
            //audio tag， 填充AF 0
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

    if(media_info_.video_codec_id == RTMP_CODEC_ID_H264)// 如果是H.264编码
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
            data[index++] = media_info_.sps.get()[1];// sps profile
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

/**
 * @brief 打开 RTMP 链接并执行握手
 * @param url RTMP 推流地址
 * @param msec 连接超时时间（毫秒）
 * @return 0 成功，-1 失败
 */
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

/**
 * @brief 推送 H.264 视频帧
 * @param data NAL 单元数据缓冲
 * @param size 数据长度（字节）
 * @return >=0 表示已发送字节数，<0 表示失败，0 表示丢弃非关键帧
 */
int RtmpPublisher::PushVideoFrame(uint8_t *data, uint32_t size)
{
    if(rtmp_conn_ == nullptr || rtmp_conn_->IsClosed() || size <= 5)
    {
        return -1;
    }

    if(media_info_.video_codec_id == RTMP_CODEC_ID_H264)
    {
        //是否已经发送第一个包
        if(!has_key_frame_)//如果没有发送过关键帧
            {
            if(this->IsKeyFrame(data,size))// 如果是关键帧
            {
                has_key_frame_ = true;
                rtmp_conn_->SendVideoData(0,avc_sequence_header_,avc_sequence_header_size_);// 发送 SPS/PPS 序列头
                rtmp_conn_->SendVideoData(0,aac_sequence_header_,aac_sequence_header_size_);// 发送 AAC 序列头
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

/**
 * @brief 推送 AAC 音频帧
 * @param data AAC 原始帧数据
 * @param size 数据长度（字节）
 * @return 0 成功，<0 失败
 */
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
        //填充字段 AF 01
        playload.get()[0] = 0xAF;
        playload.get()[1] = 1;
        memcpy(playload.get() + 2 ,data,size);
        rtmp_conn_->SendAudioData(timestamp,playload,play_size);// 发送音频数据
    }
    return 0;
}

/**
 * @brief 关闭 RTMP 推流连接并复位状态
 */
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

/**
 * @brief 检查 RTMP 连接状态
 * @return true 表示已连接，false 表示未连接或已关闭
 */
bool RtmpPublisher::IsConnected()
{
    if(rtmp_conn_)
    {
        return (!rtmp_conn_->IsClosed());
    }
    return false;
}

/**
 * @brief 判断 H.264 NAL 数据是否为关键帧
 * @param data NAL 数据缓冲指针
 * @param size 缓冲长度
 * @return true 表示关键帧，false 表示非关键帧
 */
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

