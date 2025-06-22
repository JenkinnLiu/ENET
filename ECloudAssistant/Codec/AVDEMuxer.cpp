/**
 * @file AVDEMuxer.cpp
 * @brief AVDEMuxer 类实现
 *
 * 实现了基于 FFmpeg 的媒体流打开、异步读取、音视频流分发
 * 到 AAC 和 H.264 解码器的完整过程。
 */

#include "AVDEMuxer.h"
#include "H264_Decoder.h"
#include "AAC_Decoder.h"

/**
 * @brief 构造函数
 * @param ac 外部 AVContext，用于保存音视频时长等信息
 *
 * 初始化解码器实例、创建格式化上下文并设置中断回调和字典选项。
 */
AVDEMuxer::AVDEMuxer(AVContext *ac)
    :avContext_(ac)
    ,avDict_(nullptr)
    ,readthread_(nullptr)
{
    aacDecoder_.reset(new AAC_Decoder(avContext_));
    h264Decoder_.reset(new H264_Decoder(avContext_));
    //设置字典
//    av_dict_set(&avDict_,"stimeout","1000000",0);
//    av_dict_set(&avDict_,"analyzeduration","0",0);//设为0可以快速播放
//    av_dict_set(&avDict_,"fflags","nobuffer",0);//可以快速播放
//    av_dict_set(&avDict_,"flags","low_delay",0);//无延迟
//    av_dict_set(&avDict_,"tune","zerolatency",0);//零延迟

    av_dict_set(&avDict_, "stimeout", "1000000", 0);
    av_dict_set(&avDict_, "analyzeduration", "0", 0);
    av_dict_set(&avDict_, "max_analyze_duration","10",0);
    av_dict_set(&avDict_, "fflags", "nobuffer", 0);
    av_dict_set(&avDict_, "flags", "low_delay", 0);
    av_dict_set(&avDict_, "tune","zerolatency",0);
    av_dict_set(&avDict_, "reconnect", "0", 0);
    av_dict_set(&avDict_, "reconnect_streamed", "0", 0);
    av_dict_set(&avDict_, "reconnect_delay_max", "5", 0);

    //创建这个封装格式上下文
    pFormateCtx_ = avformat_alloc_context();
    //设置参数
    pFormateCtx_->max_ts_probe = 50;
    pFormateCtx_->probesize = 500000;
    pFormateCtx_->interrupt_callback.callback = InterruptFouction;//通过这个回调函数来去退出
    pFormateCtx_->interrupt_callback.opaque = this;
    pFormateCtx_->flags |= AVFMT_FLAG_DISCARD_CORRUPT;//加速
}

/**
 * @brief 析构函数
 *
 * 调用 Close() 停止读取线程并释放所有内部资源。
 */
AVDEMuxer::~AVDEMuxer()
{
    Close();
}

/**
 * @brief 打开媒体流并启动异步读取线程
 * @param path 流媒体地址或文件路径
 * @return 始终返回 true（线程启动成功）
 */
bool AVDEMuxer::Open(const std::string &path)
{
    //启动线程去查询流消息
    readthread_.reset(new std::thread([this,path](){
        this->FetchStream(path);
    }));
    return true;
}

/**
 * @brief 关闭读取线程并释放资源
 *
 * 设置退出标志，释放字典、等待线程结束并关闭格式化上下文。
 */
void AVDEMuxer::Close()
{
    //将退出标志置为true;
    quit_ = true;
    //释放字典
    if(avDict_)
    {
        av_dict_free(&avDict_);
    }
    //回收线程资源
    if(readthread_)
    {
        if(readthread_->joinable())
        {
            readthread_->join();
            readthread_.reset();
            readthread_ = nullptr;
        }
    }
    //释放这个封装格式上下文
    if(pFormateCtx_)
    {
        avformat_close_input(&pFormateCtx_);
        pFormateCtx_ = nullptr;
    }

}

/**
 * @brief 异步读取媒体包并分发给对应解码器
 * @param path 流媒体地址或文件路径
 *
 * 在单独线程中调用，先获取流信息回调，然后循环读取
 * AVPacket 并根据 stream_index 转发给 h264Decoder_ 或 aacDecoder_。
 */
void AVDEMuxer::FetchStream(const std::string &path)
{
    bool ret = true;
    //读流信息
    //返回这个流查询结果
    if(streamCb_)
    {
        ret = FetchStreamInfo(path);
        streamCb_(ret);
    }
    if(!ret)
    {
        return;
    }
    //如果这个结果为true就是有流信息，我们开始读取数据；
    AVPacketPtr packet = nullptr;
    while(!quit_ && pFormateCtx_)
    {
        packet = AVPacketPtr(av_packet_alloc(),[](AVPacket* p){av_packet_free(&p);});
        //在开始读取
        int r = av_read_frame(pFormateCtx_,packet.get());
        if(r == 0) //读取成功
        {
            if(packet->stream_index == videoIndex)//h264数据包
            {
                //将数据传到这个264解码器队列中
                h264Decoder_->put_packet(packet);
            }
            else if(packet->stream_index == audioIndex)
            {
                //将数据传到AAC解码器队列
                aacDecoder_->put_packet(packet);
            }
            else
            {
                //释放包
                av_packet_unref(packet.get());
            }
        }
        else
        {
            av_packet_unref(packet.get());
            break;
        }
    }

}

/**
 * @brief 打开并解析流信息，查找音视频流并初始化解码器
 * @param path 流媒体地址或文件路径
 * @return 成功返回 true，失败返回 false
 */
bool AVDEMuxer::FetchStreamInfo(const std::string &path)
{
    //获取流信息
    if(avformat_open_input(&pFormateCtx_,path.c_str(),nullptr,&avDict_) != 0)
    {
        return false;
    }
    //查询流信息
    if(avformat_find_stream_info(pFormateCtx_,nullptr) < 0)//查询失败
    {
        return false;
    }
    //遍历流
    for(int i = 0;i < pFormateCtx_->nb_streams;i++)
    {
        if(pFormateCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            //更新视频流索引
            videoIndex = i;
            //时长
            avContext_->videoDuration = pFormateCtx_->streams[i]->duration * av_q2d(pFormateCtx_->streams[i]->time_base);
        }
        else if(pFormateCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            //更新视频流索引
            audioIndex = i;
            //时长
            avContext_->audioDuration = pFormateCtx_->streams[i]->duration * av_q2d(pFormateCtx_->streams[i]->time_base);
        }
    }

    if(videoIndex != -1) //存在视频流
    {
        //初始化视频解码器
        if(h264Decoder_->Open(pFormateCtx_->streams[videoIndex]->codecpar) != 0)
        {
            h264Decoder_.reset();
            h264Decoder_ = nullptr;
            return false;
        }
    }
    if(audioIndex != -1) //存在音频
    {
        //初始化音频解码器
        if(aacDecoder_->Open(pFormateCtx_->streams[audioIndex]->codecpar) != 0)
        {
            aacDecoder_.reset();
            aacDecoder_ = nullptr;
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取音频总时长（秒）
 * @return 音频时长
 */
double AVDEMuxer::audioDuration()
{
    //音频时长
    return avContext_->audioDuration;
}

/**
 * @brief 获取视频总时长（秒）
 * @return 视频时长
 */
double AVDEMuxer::videoDuration()
{
    return avContext_->videoDuration;
}

/**
 * @brief 中断回调函数，用于异步读取时检测退出标志
 * @param arg 指向 AVDEMuxer 实例的指针
 * @return 非零值表示中断读取
 */
int AVDEMuxer::InterruptFouction(void *arg)
{
    //退出标志
    AVDEMuxer* thiz = (AVDEMuxer*)arg;
    return thiz->quit_; //0 false 1 true;
}
