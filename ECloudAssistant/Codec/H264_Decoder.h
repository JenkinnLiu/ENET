/**
 * @file H264_Decoder.h
 * @brief H.264 视频解码器类定义
 *
 * 封装了基于 FFmpeg 的 H.264 码流解码及帧格式转换，
 * 提供异步线程解码和将 YUV 帧推送到 AVContext 的功能。
 */

#ifndef H264_DECODER_H
#define H264_DECODER_H
#include <QThread>
#include "AV_Common.h"

class VideoConverter;
/**
 * @class H264_Decoder
 * @brief 异步 H.264 视频解码器
 *
 * 继承自 QThread 和 DecodBase，负责从 AVPacket 队列中获取 H.264 数据，
 * 解码为 AVFrame 后通过 VideoConverter 转换为指定像素格式，
 * 最终将 YUV 帧推送到 AVContext 的视频队列。
 */
class H264_Decoder : public QThread ,public DecodBase
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param ac 外部 AVContext 指针，用于存储解码后的视频帧和时长
     * @param parent Qt 父对象指针，默认为 nullptr
     */
    H264_Decoder(AVContext* ac, QObject* parent = nullptr);

    /**
     * @brief 析构函数，停止解码线程并释放资源
     */
    ~H264_Decoder();

    /**
     * @brief 打开 H.264 解码器并初始化转换器
     * @param codecParamer 解码参数（AVCodecParameters），包含宽高、像素格式等
     * @return 成功返回 0，失败返回 -1
     */
    int  Open(const AVCodecParameters* codecParamer);

    /**
     * @brief 判断解码队列是否已满
     * @return 队列长度超过 10 时返回 true，否则返回 false
     */
    inline bool isFull(){return video_queue_.size() > 10;}

    /**
     * @brief 将 H.264 数据包加入解码队列
     * @param packet AVPacket 智能指针，持有 H.264 码流数据
     */
    inline void put_packet(const AVPacketPtr packet){video_queue_.push(packet);}

protected:
    /**
     * @brief 关闭解码器，设置退出标志并停止线程
     */
    void Close();

    /**
     * @brief 线程入口函数，执行异步解码循环
     */
    virtual void run()override;
private:
    bool quit_ = false;                       /**< 退出标志 */
    AVFramePtr yuv_frame_ = nullptr;        /**< YUV 帧指针 */
    AVQueue<AVPacketPtr> video_queue_;      /**< 视频数据包队列 */
    AVContext*    avContext_ = nullptr;     /**< AVContext 指针 */
    std::unique_ptr<VideoConverter> videoConver_; /**< 视频格式转换器 */
};

#endif // H264_DECODER_H
