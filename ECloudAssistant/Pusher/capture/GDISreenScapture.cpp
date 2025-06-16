#include "GDISreenScapture.h"
//添加ffmpeg头文件
extern "C"
{
    #include <libavcodec/avcodec.h>
    #include<libavdevice/avdevice.h>
    #include<libavformat/avformat.h>
    #include<libswscale/swscale.h>
}
#include <QDebug>

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

GDIScreenCapture::~GDIScreenCapture()
{
    Close();
}

quint32 GDIScreenCapture::GetWidth() const
{
    return width_;
}

quint32 GDIScreenCapture::GetHeight() const
{
    return height_;
}

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
    for(uint32_t i = 0;i<format_context_->nb_streams;i++)
    {
        if(format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
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
    codec_context_->pix_fmt = AV_PIX_FMT_RGBA;
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
    rgba.assign(rgba_frame_.get(),rgba_frame_.get() + frame_size_);
    width = width_;
    height = height_;
    return true;
}

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

bool GDIScreenCapture::Decode(AVFrame *av_frame, AVPacket *av_packet)
{
    int ret = avcodec_send_packet(codec_context_,av_packet);
    if(ret < 0)
    {
        return false;
    }
    if(ret >= 0)
    {
        //接收frmae
        ret = avcodec_receive_frame(codec_context_,av_frame);
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
        rgba_frame_.reset(new uint8_t[frame_size_],std::default_delete<uint8_t[]>());
        width_ = av_frame->width;
        height_ = av_frame->height;
        //将frame数据拷贝出去
        for(uint32_t i = 0;i<height_;i++)
        {
            memcpy(rgba_frame_.get() + i * width_ * 4 ,av_frame->data[0] + i * av_frame->linesize[0],av_frame->linesize[0]);
        }
        av_frame_unref(av_frame);
    }
    return true;
}
