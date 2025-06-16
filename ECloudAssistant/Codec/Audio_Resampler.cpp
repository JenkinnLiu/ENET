#include "Audio_Resampler.h"

extern "C"
{
    #include<libswresample/swresample.h>
    #include<libavutil/opt.h>
    #include<libavutil/channel_layout.h>
    #include<libavutil/samplefmt.h>
}

AudioResampler::AudioResampler()
    :swr_context_(nullptr)
{

}

AudioResampler::~AudioResampler()
{
    Close();
}

void AudioResampler::Close()
{
    if(swr_context_)
    {
        if(swr_is_initialized(swr_context_))
        {
            swr_close(swr_context_);
            swr_context_ = nullptr;
        }
    }
}

int AudioResampler::Convert(AVFramePtr in_frame, AVFramePtr &out_frame)
{
    //重采样
    if(!swr_context_)
    {
        return -1;
    }

    //更新输出参数
    out_frame.reset(av_frame_alloc(),[](AVFrame* ptr){av_frame_free(&ptr);});
    out_frame->sample_rate = out_samplerate_;
    out_frame->format = out_format_;
    out_frame->channels = out_channels_;
    int64_t delay = swr_get_delay(swr_context_,in_frame->sample_rate);
    out_frame->nb_samples = av_rescale_rnd(delay + in_frame->nb_samples,out_samplerate_,in_frame->sample_rate,AV_ROUND_UP);
    out_frame->pts = out_frame->pkt_dts = in_frame->pts;

    //获取内存
    if(av_frame_get_buffer(out_frame.get(),0) != 0)
    {
        return -1;
    }

    //开始重采样
    int len = swr_convert(swr_context_,(uint8_t**)&out_frame->data,out_frame->nb_samples,(const uint8_t**)in_frame->data,in_frame->nb_samples);
    if(len < 0)
    {
        //清空内存
        out_frame.reset();
        out_frame = nullptr;
        return -1;
    }
    //更新实际样品数
    out_frame->nb_samples = len;
    return len;
}

bool AudioResampler::Open(int in_samplerate, int in_channels, AVSampleFormat in_format, int out_samplerate, int out_channels, AVSampleFormat out_format)
{
    if(swr_context_)
    {
        return false;
    }

    //初始化转换器
    int64_t in_channels_layout = av_get_default_channel_layout(in_channels);
    int64_t out_channels_layout = av_get_default_channel_layout(out_channels);

    //创建转换器
    swr_context_ = swr_alloc();

    //设置参数
    av_opt_set_int(swr_context_,"in_channel_layout",in_channels_layout,0);
    av_opt_set_int(swr_context_,"in_sample_rate",in_samplerate,0);
    av_opt_set_int(swr_context_,"in_sample_fmt",in_format,0);

    av_opt_set_int(swr_context_,"out_channel_layout",out_channels_layout,0);
    av_opt_set_int(swr_context_,"out_sample_rate",out_samplerate,0);
    av_opt_set_int(swr_context_,"out_sample_fmt",out_format,0);

    int ret = swr_init(swr_context_);
    if(ret < 0)
    {
        return false;
    }

    in_samplerate_ = in_samplerate;
    in_channels_ = in_channels;
    in_bits_per_sample_ = av_get_bytes_per_sample(in_format);
    in_format_ = in_format;

    out_samplerate_ = out_samplerate;
    out_channels_ = out_channels;
    out_bits_per_sample_ = av_get_bytes_per_sample(out_format);
    out_format_ = out_format;
    return true;
}
