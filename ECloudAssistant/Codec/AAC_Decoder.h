/**
 * @file AAC_Decoder.h
 * @brief AAC 音频解码器类定义
 *
 * 封装了基于 FFmpeg 的 AAC 音频解码流程，
 * 支持异步线程解码、音频重采样并将 PCM 帧推送到 AVContext 队列。
 */

#ifndef AAC_DECODER_H
#define AAC_DECODER_H
#include <QThread>
#include "AV_Common.h"

class AudioResampler;
class AAC_Decoder : public QThread ,public DecodBase
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param ac 外部 AVContext 指针，用于存储解码后的 PCM 帧和时长信息
     * @param parent Qt 父对象指针，默认为 nullptr
     */
    AAC_Decoder(AVContext* ac,QObject* parent = nullptr);

    /**
     * @brief 析构函数，停止解码线程并释放资源
     */
    ~AAC_Decoder();

    /**
     * @brief 打开解码器并初始化解码上下文和重采样器
     * @param codecParamer 音频流参数（AVCodecParameters）
     * @return 成功返回 0，失败返回 -1
     */
    int Open(const AVCodecParameters* codecParamer);

    /**
     * @brief 将 AAC 数据包加入解码队列
     * @param packet AVPacket 智能指针，持有 AAC 编码数据
     */
    inline void put_packet(const AVPacketPtr packet){audio_queue_.push(packet);}

    /**
     * @brief 检查解码队列是否已满
     * @return 当队列长度超过 50 返回 true，否则返回 false
     */
    inline bool isFull(){return audio_queue_.size() > 50;}

protected:
    /**
     * @brief 关闭解码器，停止解码线程
     */
    void Close();

    /**
     * @brief 线程入口函数，执行异步解码循环
     */
    virtual void run()override;
private:
    bool            quit_ = false;
    AVQueue<AVPacketPtr>   audio_queue_;
    AVContext*      avContext_ = nullptr;
    std::unique_ptr<AudioResampler> audioResampler_;
};
#endif // AAC_DECODER_H
