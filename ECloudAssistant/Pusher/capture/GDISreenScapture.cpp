/**
 * @file GDISreenScapture.cpp
 * @brief 实现 GDIScreenCapture 类的屏幕采集逻辑，使用 FFmpeg gdigrab 获取并解码屏幕图像
 * @date 2025-06-18
 */
#include "GDISreenScapture.h"
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}
#include <QDebug>

/**
 * @brief 构造函数，注册 FFmpeg 设备并分配帧/包缓冲区
 */
GDIScreenCapture::GDIScreenCapture()
    :stop_(false)
    ,is_initialzed_(false)
    ,frame_size_(0)
    ,rgba_frame_(nullptr)
    ,width_(0)
    ,height_(0)
    ,video_index_(-1)
    ,framerate_(25)
    ,input_format_(nullptr)
    ,codec_context_(nullptr)
    ,format_context_(nullptr)
    ,av_frame_(nullptr)
    ,av_packet_(nullptr)
{
    //注册设备,使用gdi采集
    avdevice_register_all();
    //创建frmae和packet
    av_frame_ = std::shared_ptr<AVFrame>(av_frame_alloc(),[](AVFrame* ptr){av_frame_free(&ptr);});
    av_packet_ = std::shared_ptr<AVPacket>(av_packet_alloc(),[](AVPacket* ptr){av_packet_free(&ptr);});

}

/**
 * @brief 析构函数，释放资源并停止采集
 */
GDIScreenCapture::~GDIScreenCapture()
{
    Close();
}

/**
 * @brief 获取当前屏幕宽度
 * @return 屏幕宽度
 */
quint32 GDIScreenCapture::GetWidth() const
{
    return width_;
}

/**
 * @brief 获取当前屏幕高度
 * @return 屏幕高度
 */
quint32 GDIScreenCapture::GetHeight() const
{
    return height_;
}

/**
 * @brief 初始化屏幕采集，打开输入设备并配置解码器
 * @param display_index 屏幕索引（默认 0）
 * @return true 初始化成功，false 失败
 */
bool GDIScreenCapture::Init(qint64 display_index)
{
    if(is_initialzed_)
    {
        return true;
    }

    AVDictionary *options = nullptr;
    //设置属性
    av_dict_set_int(&options,"framerate",framerate_,AV_DICT_MATCH_CASE);//设置采集帧率
    av_dict_set_int(&options,"draw_mouse",1,AV_DICT_MATCH_CASE); //绘制鼠标
    av_dict_set_int(&options,"offset_x",0,AV_DICT_MATCH_CASE);
    av_dict_set_int(&options,"offset_y",0,AV_DICT_MATCH_CASE);
    av_dict_set(&options,"video_size","2560x1440",1);//屏幕分辨率

    //创建输入format
    input_format_ = const_cast<AVInputFormat*>(av_find_input_format("gdigrab"));//gdi
    if(!input_format_)
    {
        qDebug() << "av_find_input_format failed";
        return false;
    }

    //创建格式上下文
    format_context_ = avformat_alloc_context();
    if(avformat_open_input(&format_context_,"desktop",input_format_,&options) != 0)
    {
        qDebug() << "avformat_open_input failed";
        return false;
    }

    //查询流信息
    if(avformat_find_stream_info(format_context_,nullptr) < 0)
    {
        qDebug() << "avformat_find_stream_info failed";
        return false;
    }

    //找到视频流，来采集视频
    int video_index = -1;
    //遍历所有流，找到视频流
    for(uint32_t i = 0;i<format_context_->nb_streams;i++)
    {
        //检查流类型
        if(format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;//记录视频流索引
        }
    }

    if(video_index == -1)
    {
        //没有视频流
        return false;
    }

    //创建解码器
    AVCodec* codec = const_cast<AVCodec*>(avcodec_find_decoder(format_context_->streams[video_index]->codecpar->codec_id));
    if(!codec)
    {
        return false;
    }

    //创建解码器上下文
    codec_context_ = avcodec_alloc_context3(codec);
    if(!codec_context_)
    {
        return false;
    }

    //为解码器上下文设置参数
    codec_context_->pix_fmt = AV_PIX_FMT_RGBA;//设置像素格式为 RGBA
    //我们需要去复制解码器上下文
    avcodec_parameters_to_context(codec_context_,format_context_->streams[video_index]->codecpar);
    //打开解码器
    if(avcodec_open2(codec_context_,codec,nullptr) != 0)
    {
        return false;
    }

    //初始化成功
    video_index_ = video_index;
    is_initialzed_ = true;
    //启动线程来去捕获视频流
    this->start(); //因为我们继承QThread，所有通过start可以去执行run
    return true;
}

/**
 * @brief 关闭采集，停止线程并释放 FFmpeg 相关上下文
 * @return true 关闭成功
 */
bool GDIScreenCapture::Close()
{
    if(is_initialzed_)
    {
        StopCapture();
    }
    if(codec_context_)
    {
        avcodec_close(codec_context_);
        codec_context_ = nullptr;
    }
    if(format_context_)
    {
        avformat_close_input(&format_context_);
        format_context_ = nullptr;
    }
    input_format_ = nullptr;
    video_index_ = -1;
    is_initialzed_ = false;
    stop_ = true; //停掉线程
    return true;
}

/**
 * @brief 获取已捕获帧的 RGBA 数据和分辨率
 * @param rgba 输出像素数据容器
 * @param width 返回的帧宽度
 * @param height 返回的帧高度
 * @return true 成功获取帧，false 无可用帧或错误
 */
bool GDIScreenCapture::CaptureFrame(FrameContainer &rgba, quint32 &width, quint32 &height)
{
    //捕获视频帧
    std::lock_guard<std::mutex> lock(mutex_);
    if(stop_)
    {
        rgba.clear();
    }

    if(rgba_frame_ == nullptr || frame_size_ == 0)//帧错误
    {
        rgba.clear();
        return false;
    }
    if(rgba.capacity() < frame_size_)
    {
        //扩容
        rgba.reserve(frame_size_);
    }
    //拷贝帧数据
    rgba.assign(rgba_frame_.get(),rgba_frame_.get() + frame_size_);// 从缓冲区拷贝数据到 rgba
    width = width_;
    height = height_;
    return true;
}

/**
 * @brief QThread 入口，按帧率循环调用 GetOneFrame 捕获并解码帧
 */
void GDIScreenCapture::run()
{
    //线程 我们需要使用gdi来获取视频数据
    if(is_initialzed_ && !stop_)
    {
        while (!stop_) {
            //每秒25张
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / framerate_));
            GetOneFrame();
        }
    }
}

/**
 * @brief 停止后台采集线程并清空帧缓冲
 */
void GDIScreenCapture::StopCapture()
{
    if(is_initialzed_)
    {
        stop_ = true;
        if(this->isRunning())
        {
            this->quit();
            this->wait();
        }
        //清空这个帧数据
        std::lock_guard<std::mutex> lock(mutex_);
        rgba_frame_.reset();
        frame_size_ = 0;
        width_ = 0;
        height_ = 0;
    }
}

/**
 * @brief 从 FFmpeg 流读取一个 AVPacket，并调用 Decode 进行解码
 * @return true 成功读取并解码，false 失败或无更多数据
 */
bool GDIScreenCapture::GetOneFrame()
{
    //获取帧数据
    if(stop_)
    {
        return false;
    }
    //我们就去读一帧
    int ret = av_read_frame(format_context_,av_packet_.get());
    if(ret < 0)
    {
        return false;
    }

    if(av_packet_->stream_index == video_index_) //视频流
    {
        Decode(av_frame_.get(),av_packet_.get());
    }
    av_packet_unref(av_packet_.get());
    return true;
}

/**
 * @brief 解码输入 AVPacket 为 AVFrame，并将数据按行拷贝到 RGBA 缓冲
 * @param av_frame 用于接收解码后的帧结构
 * @param av_packet 输入的压缩数据包
 * @return true 成功解码，false 失败
 */
bool GDIScreenCapture::Decode(AVFrame *av_frame, AVPacket *av_packet)
{
    //解码一帧数据
    int ret = avcodec_send_packet(codec_context_,av_packet);// 发送包到解码器
    if(ret < 0)
    {
        return false;
    }
    if(ret >= 0)
    {
        //接收frame
        ret = avcodec_receive_frame(codec_context_,av_frame);// 从解码器接收解码后的帧
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return false;
        }
        if(ret < 0)
        {
            return false;
        }
        //接收成功，需要更新帧
        std::lock_guard<std::mutex> lock(mutex_);
        frame_size_ = av_frame->pkt_size;
        rgba_frame_.reset(new uint8_t[frame_size_],std::default_delete<uint8_t[]>());// 分配 RGBA 帧缓冲
        width_ = av_frame->width;//  设置宽度
        height_ = av_frame->height; // 设置高度
        //将frame数据拷贝出去
        for(uint32_t i = 0;i<height_;i++)
        {
            // 将每行数据从 AVFrame 拷贝到 RGBA 缓冲
            memcpy(rgba_frame_.get() + i * width_ * 4 ,av_frame->data[0] + i * av_frame->linesize[0],av_frame->linesize[0]);
        }
        av_frame_unref(av_frame);// 释放 AVFrame 的引用
    }
    return true;
}
