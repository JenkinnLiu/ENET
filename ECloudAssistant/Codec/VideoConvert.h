/**
 * @file VideoConvert.h
 * @brief H264视频帧像素格式与分辨率转换类定义
 *
 * 定义了 VideoConverter 类，用于基于 FFmpeg libswscale 对视频帧进行像素格式和分辨率的转换。
 */

#ifndef VIDEO_CONVERTER_H
#define VIDEO_CONVERTER_H
#include "AV_Common.h"
struct SwsContext;
/**
 * @class VideoConverter
 * @brief 视频帧转换器
 *
 * 封装 FFmpeg libswscale 的 SwsContext，提供打开、关闭和帧转换接口。
 */
class VideoConverter
{
public:
    /**
     * @brief 构造函数，初始化成员变量
     */
    VideoConverter();

    /**
     * @brief 析构函数，释放转换器资源
     */
    virtual ~VideoConverter();

    /**
     * @brief 禁用拷贝构造和赋值运算符
     */
    VideoConverter(const VideoConverter&) = delete;
    VideoConverter& operator=(const VideoConverter&) = delete;
public:
    /**
     * @brief 打开并初始化转换器
     * @param in_width 输入帧宽度
     * @param in_height 输入帧高度
     * @param in_format 输入帧像素格式
     * @param out_width 输出帧宽度
     * @param out_height 输出帧高度
     * @param out_format 输出帧像素格式
     * @return 成功返回 true，失败返回 false
     */
    bool Open(qint32 in_width,qint32 in_height,AVPixelFormat in_format,
              qint32 out_width,qint32 out_height,AVPixelFormat out_format);

    /**
     * @brief 关闭并释放转换器
     */
    void Close();

    /**
     * @brief 对单帧进行格式转换
     * @param in_frame 输入 AVFramePtr 智能指针
     * @param out_frame 输出 AVFramePtr 智能指针
     * @return 转换成功返回处理行数（>=0），失败返回 -1
     */
    qint32 Convert(AVFramePtr in_frame, AVFramePtr& out_frame);
private:
    qint32 width_;          ///< 输出帧宽度
    qint32 height_;         ///< 输出帧高度
    AVPixelFormat format_;  ///< 输出帧像素格式
    SwsContext* swsContext_;///< libswscale 上下文指针
};
#endif // VIDEO_CONVERTER_H
