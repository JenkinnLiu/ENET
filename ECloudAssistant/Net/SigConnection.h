/**
 * @file SigConnection.h
 * @brief 定义了客户端的信令连接类 SigConnection。
 * @details
 * 此类继承自 TcpConnection，专门负责处理客户端与信令服务器之间的所有通信。
 * 它管理连接状态、用户类型（控制端/被控端），并根据从服务器收到的指令
 * 执行相应的本地操作，如启动/停止推流、播放视频流以及模拟远程的鼠标键盘事件。
 * 它是客户端实现远程控制逻辑的核心网络模块。
 */
#ifndef SIGCONNECTION_H
#define SIGCONNECTION_H
#include<functional>
#include "BufferReader.h"
#include "TcpConnection.h"
#include <QtGlobal>
#include <QScreen>
#include <QCursor>
#include "AV_Queue.h"

struct packet_head;

/**
 * @class SigConnection
 * @brief 客户端信令连接处理类。
 * @details
 * 封装了与信令服务器的TCP连接。它处理消息的收发，并根据消息类型
 * 调用不同的处理函数。通过回调函数与上层业务逻辑（如推流器、播放器）解耦。
 */
class SigConnection : public TcpConnection
{
public:
    /**
     * @enum UserType
     * @brief 定义了客户端的身份类型。
     */
    enum UserType
    {
        CONTROLLED, ///< 被控端，负责推流和接收远程控制事件。
        CONTROLLING ///< 控制端，负责拉流和发送控制事件。
    };

    /**
     * @enum State
     * @brief 定义了客户端在信令交互中的状态。
     */
    enum State
    {
        NONE,   ///< 初始状态，尚未加入房间。
        IDLE,   ///< 空闲状态，已成功加入房间，等待下一步指令。
        PULLER, ///< 拉流状态，作为控制端正在请求或播放视频流。
        PUSHER  ///< 推流状态，作为被控端正在推送视频流。
    };

    /**
     * @brief 构造函数。
     * @param scheduler 任务调度器，用于网络事件。
     * @param sockfd TCP连接的文件描述符。
     * @param code 识别码。对于被控端，是其唯一ID；对于控制端，是其想要连接的被控端的ID。
     * @param type 客户端的身份类型 (CONTROLLED 或 CONTROLLING)。
     */
    SigConnection(TaskScheduler* scheduler, int sockfd,const QString& code,const UserType& type = CONTROLLED);

    /**
     * @brief 析构函数。
     */
    virtual ~SigConnection();

    /**
     * @brief 检查当前是否处于空闲状态。
     * @return 如果是，返回 true。
     */
    inline bool isIdle(){return state_ == IDLE;}

    /**
     * @brief 检查当前是否处于推流状态。
     * @return 如果是，返回 true。
     */
    inline bool isPusher(){return state_ == PUSHER;}

    /**
     * @brief 检查当前是否处于拉流状态。
     * @return 如果是，返回 true。
     */
    inline bool isPuller(){return state_ == PULLER;}

    /**
     * @brief 检查当前是否处于初始状态。
     * @return 如果是，返回 true。
     */
    inline bool isNone(){return state_ == NONE;}

    /// @brief 停止推流/拉流的回调函数类型。
    using StopStreamCallBack = std::function<void()>;
    /// @brief 启动推流/拉流的回调函数类型。参数为流地址，返回是否成功。
    using StartStreamCallBack = std::function<bool(const QString& streamAddr)>;

    /**
     * @brief 设置启动流的回调函数。
     * @param cb 回调函数对象。
     */
    inline void SetStartStreamCallBack(const StartStreamCallBack& cb){startStreamCb_ = cb;}

    /**
     * @brief 设置停止流的回调函数。
     * @param cb 回调函数对象。
     */
    inline void SetStopStreamCallBack(const StopStreamCallBack& cb){stopStreamCb_ = cb;}

protected:
    /**
     * @brief 读事件回调函数，由基类TcpConnection调用。
     * @param buffer 包含接收到数据的缓冲区。
     * @return 总是返回 true。
     */
    bool OnRead(BufferReader& buffer);

    /**
     * @brief 连接关闭回调函数，由基类TcpConnection调用。
     */
    void OnClose();

    /**
     * @brief 消息分发处理器。
     * @details 从缓冲区中解析出完整的消息包，并根据消息类型调用相应的处理函数。
     * @param buffer 数据缓冲区。
     */
    void HandleMessage(BufferReader& buffer);

private:
    /**
     * @brief 发送加入房间的请求到服务器。
     * @return 成功返回0，否则返回-1。
     */
    qint32 Join();

    /**
     * @brief (仅控制端) 发送获取视频流的请求到服务器。
     * @return 成功返回0，否则返回-1。
     */
    qint32 obtainStream();

    /**
     * @brief 处理服务器对“加入房间”请求的响应。
     * @param data 指向收到的消息包头。
     */
    void doJoin(const packet_head* data);

    /**
     * @brief (仅控制端) 处理服务器发来的“播放流”指令。
     * @param data 指向收到的消息包头，其中包含流地址。
     */
    void doPlayStream(const packet_head* data);

    /**
     * @brief (仅被控端) 处理服务器发来的“创建流”请求。
     * @param data 指向收到的消息包头。
     */
    void doCtreatStream(const packet_head* data);

    /**
     * @brief 处理服务器发来的“删除/停止流”指令。
     * @param data 指向收到的消息包头。
     */
    void doDeleteStream(const packet_head* data);

private:
    /**
     * @brief (仅被控端) 处理鼠标点击（按下/释放）事件。
     * @param data 包含鼠标事件信息的消息包。
     */
    void doMouseEvent(const packet_head* data);

    /**
     * @brief (仅被控端) 处理鼠标移动事件。
     * @param data 包含鼠标位置比例信息的消息包。
     */
    void doMouseMoveEvent(const packet_head* data);

    /**
     * @brief (仅被控端) 处理键盘事件。
     * @param data 包含键盘按键信息的消息包。
     */
    void doKeyEvent(const packet_head* data);

    /**
     * @brief (仅被控端) 处理鼠标滚轮事件。
     * @param data 包含滚轮滚动信息的消息包。
     */
    void doWheelEvent(const packet_head* data);

private:
    bool quit_ = false; ///< 退出标志。
    State state_; ///< 当前的信令状态。
    QString code_ = ""; ///< 识别码。
    const UserType type_; ///< 客户端的身份类型。
    QScreen* screen_ = nullptr; ///< 指向主屏幕的指针，用于获取分辨率。
    StopStreamCallBack stopStreamCb_ = [](){}; ///< 停止流的回调函数。
    StartStreamCallBack startStreamCb_ = [](const QString&)->bool{return true;}; ///< 启动流的回调函数。
    std::unique_ptr<std::thread> eventthread_ = nullptr; ///< (未使用) 事件处理线程指针。
};
#endif // SIGCONNECTION_H
