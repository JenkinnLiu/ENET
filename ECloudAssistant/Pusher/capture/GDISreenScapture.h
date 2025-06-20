/**
 * @file GDISreenScapture.h
 * @brief 基于 FFmpeg 和 GDI 的屏幕捕获类声明，通过 QThread 后台采集屏幕帧并提供 RGBA 数据
 * @date 2025-06-18
 */
#ifndef GDISCREENCAPTURE_H
#define GDISCREENCAPTURE_H

#include <QThread>
#include <memory>
#include <mutex>

struct AVFrame;
struct AVPacket;
struct AVInputFormat;
struct AVCodecContext;
struct AVFormatContext;
using FrameContainer = std::vector<quint8>;

/**
 * @class GDIScreenCapture
 * @brief 继承 QThread 的屏幕采集器，使用 FFmpeg 的 gdigrab 设备获取屏幕图像并解码为 RGBA 格式
 */
class GDIScreenCapture : public QThread
{
public:
    /**
     * @brief 构造函数，注册设备并初始化 FFmpeg 解码上下文
     */
    GDIScreenCapture();

    /**
     * @brief 禁用拷贝构造，防止多线程冲突
     */
    GDIScreenCapture(const GDIScreenCapture&) = delete;

    /**
     * @brief 禁用拷贝赋值运算符
     */
    GDIScreenCapture& operator=(const GDIScreenCapture&) = delete;

    /**
     * @brief 析构函数，停止采集并释放资源
     */
    virtual ~GDIScreenCapture();

    /**
     * @brief 获取捕获帧的宽度
     * @return 当前帧宽度（像素）
     */
    virtual quint32 GetWidth() const;

    /**
     * @brief 获取捕获帧的高度
     * @return 当前帧高度（像素）
     */
    virtual quint32 GetHeight() const;

    /**
     * @brief 初始化屏幕捕获上下文，包括设备格式和解码器准备
     * @param display_index 屏幕索引，默认为主显示器
     * @return true 初始化成功，false 失败
     */
    virtual bool Init(qint64 display_index = 0);

    /**
     * @brief 关闭并清理采集资源，停止后台线程
     * @return true 关闭成功
     */
    virtual bool Close();

    /**
     * @brief 从内部缓存获取最新一帧的 RGBA 像素数据
     * @param rgba 输出像素数据容器
     * @param width 返回的帧宽度
     * @param height 返回的帧高度
     * @return true 成功获取帧，false 无可用帧
     */
    virtual bool CaptureFrame(FrameContainer& rgba, quint32& width, quint32& height);

protected:
    /**
     * @brief QThread 入口，后台循环采集屏幕帧
     */
    virtual void run() override;

private:
    /**
     * @brief 停止后台采集线程并清空缓存帧
     */
    void StopCapture();

    /**
     * @brief 从 FFmpeg 读取一帧原始数据并解码
     * @return true 成功读取并解码，false 失败
     */
    bool GetOneFrame();

    /**
     * @brief 对读取的 AVPacket 使用解码上下文进行解码，填充 RGBA 帧缓冲
     * @param av_frame 解码输出帧结构
     * @param av_packet 输入压缩包
     * @return true 解码成功，false 失败
     */
    bool Decode(AVFrame* av_frame, AVPacket* av_packet);

private:
    bool stop_;                                   ///< 采集停止标志
    bool is_initialzed_;                          ///< 是否完成初始化
    quint32 frame_size_;                          ///< 当前帧数据字节数
    std::shared_ptr<quint8> rgba_frame_;          ///< 解码后的 RGBA 数据缓冲
    quint32 width_;                               ///< 采集帧宽度
    quint32 height_;                              ///< 采集帧高度
    qint64 video_index_;                          ///< 视频流索引
    qint64 framerate_;                            ///< 目标采集帧率
    std::mutex mutex_;                            ///< 保护内部帧缓冲线程安全

    AVInputFormat* input_format_;                 ///< FFmpeg 输入格式
    AVCodecContext* codec_context_;               ///< 解码器上下文
    AVFormatContext* format_context_;             ///< 格式上下文
    std::shared_ptr<AVFrame> av_frame_;           ///< FFmpeg 原始帧结构
    std::shared_ptr<AVPacket> av_packet_;         ///< FFmpeg 数据包结构
};

#endif // GDISCREENCAPTURE_H
