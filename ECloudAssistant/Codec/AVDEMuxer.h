/**
 * @file AVDEMuxer.h
 * @brief 音视频流分离器类定义
 *
 * 封装了基于 FFmpeg 的媒体流打开、音视频流索引查找、解复用
 * 以及分发数据包到指定解码器的功能，并支持中断和异步读取。
 */

#ifndef AVDEMUXER_H
#define AVDEMUXER_H
#include "AV_Common.h"
#include <functional>
#include <thread>

/**
 * @class AVDEMuxer
 * @brief 管理音视频流分离与解码流程
 *
 * 支持异步读取媒体流、查找音视频流、解复用数据包并
 * 分发到对应的 AAC 和 H.264 解码器进行解码。
 */
class AVDEMuxer
{
public:
    /**
     * @brief 构造函数，初始化解复用上下文和解码器
     * @param ac 外部 AVContext 指针，用于保存时长等信息
     */
    AVDEMuxer(AVContext* ac);

    /**
     * @brief 析构函数，关闭读取线程并释放资源
     */
    ~AVDEMuxer();

    /**
     * @brief 打开媒体流并启动读取线程
     * @param path 流媒体 URL 或本地文件路径
     * @return 启动成功返回 true，否则返回 false
     */
    bool Open(const std::string& path);

    /**
     * @brief 设置流信息回调
     * @param cb 回调函数，参数表示是否成功获取流信息
     */
    using StreamCallBack = std::function<void(bool)>;
    inline void SetStreamCallBack(const StreamCallBack& cb){streamCb_ = cb;}

protected:
    /**
     * @brief 关闭读取线程并清理内部资源
     */
    void Close();

    /**
     * @brief 异步读取流数据并分发到解码器队列
     * @param path 流媒体 URL 或本地文件路径
     */
    void FetchStream(const std::string& path);

    /**
     * @brief 打开并解析媒体流信息，查找音视频流并初始化解码器
     * @param path 流媒体 URL 或本地文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool FetchStreamInfo(const std::string& path);

    /**
     * @brief 获取音频流总时长（秒）
     * @return 音频时长
     */
    double audioDuration();

    /**
     * @brief 获取视频流总时长（秒）
     * @return 视频时长
     */
    double videoDuration();

    /**
     * @brief FFmpeg 中断回调函数，根据退出标志判断是否中断
     * @param arg AVDEMuxer 对象指针
     * @return 返回非零则中断读取
     */
    static int InterruptFouction(void* arg);
private:
    int videoIndex = -1;// 视频流索引
    int audioIndex = -1;// 音频流索引
    AVContext* avContext_;// 外部传入的 AVContext，用于保存音视频时长等信息
    AVDictionary* avDict_;// 字典，用于存储流信息
    std::atomic_bool quit_ = false;// 退出标志，用于中断读取线程
    StreamCallBack streamCb_ = [](bool){};// 流信息回调函数，默认空函数
    AVFormatContext* pFormateCtx_ = nullptr;// FFmpeg 格式化上下文，用于打开和解析媒体流
    std::unique_ptr<std::thread> readthread_;// 异步读取线程
    //解码器
    std::unique_ptr<AAC_Decoder> aacDecoder_;// AAC 解码器
    std::unique_ptr<H264_Decoder> h264Decoder_;// H.264 解码器
};

#endif // AVDEMUXER_H
