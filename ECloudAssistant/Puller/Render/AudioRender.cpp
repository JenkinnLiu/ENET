#include "AudioRender.h"

AudioRender::AudioRender()
{
    audioFmt_.setCodec("audio/pcm");
    audioFmt_.setByteOrder(QAudioFormat::LittleEndian);
    audioFmt_.setSampleType(QAudioFormat::SignedInt);
}

AudioRender::~AudioRender()
{

}

int AudioRender::AvailableBytes()
{
    //获取pcm大小
    if(!audioOut_)
    {
        return -1;
    }
    return audioOut_->bytesFree() - audioOut_->periodSize(); //剩余空间 - 每次周期需要填充的字节数
}

bool AudioRender::InitAudio(int nChannels, int SampleRate, int nSampleSize)
{
    //初始化音频输出
    if(audioOut_ || is_initail_ || device_)
    {
        return true;
    }

    nSampleSize_ = nSampleSize;

    //设置格式
    audioFmt_.setChannelCount(nChannels);
    audioFmt_.setSampleRate(SampleRate);
    audioFmt_.setSampleSize(nSampleSize);

    //创建输出
    audioOut_ = new QAudioOutput(audioFmt_);
    //设置缓冲区大小
    audioOut_->setBufferSize(409600);
    //设置音量
    audioOut_->setVolume(volume_);
    //创建接入设备
    device_ = audioOut_->start();
    is_initail_ = true;
    return true;
}

void AudioRender::Write(AVFramePtr frame)
{
    //播放音频
    if(device_ && audioOut_ && nSampleSize_ != -1)
    {
        //获取frame数据
        QByteArray audioData(reinterpret_cast<char*>(frame->data[0]),(frame->nb_samples * frame->channels) * (nSampleSize_ / 8));
        //开始播放
        device_->write(audioData.data(),audioData.size());
        //释放这个frame
        av_frame_unref(frame.get());
    }
}
