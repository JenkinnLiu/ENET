/**
 * @file RemoteManager.h
 * @brief 定义了 RemoteManager 类，作为远程控制功能的中央协调器。
 * @details
 * RemoteManager 是一个核心管理类，它继承自 RtmpPushManager，因此具备了作为“被控端”推流的能力。
 * 同时，它也能够通过创建 PullerWgt 实例来启动“主控端”界面，发起远程连接。
 * 这个类根据调用不同的接口（Init 或 StartRemote），决定当前应用程序实例扮演的角色。
 * 它管理着事件循环（EventLoop）和信令连接（SigConnection），是整个远程控制模块的入口点。
 * @author [Your Name]
 * @date 2025-06-23
 * @version 1.0
 */

#ifndef REMOTEMANAGER_H
#define REMOTEMANAGER_H

#include "Pusher/RtmpPushManager.h"
#include "Puller/UI/PullerWgt.h"
#include "Net/SigConnection.h"
#include "Net/EventLoop.h"

/**
 * @class RemoteManager
 * @brief 远程控制核心管理类，协调主控端和被控端的逻辑。
 *
 * 该类通过继承 RtmpPushManager，天然具备了屏幕采集和推流的能力（作为被控端）。
 * 同时，它也提供了启动远程控制（作为主控端）的接口。
 * - 作为被控端时：调用 Init()，连接信令服务器，等待主控端的连接请求和推流指令。
 * - 作为主控端时：调用 StartRemote()，创建拉流窗口，主动发起连接。
 *
 * 通过删除拷贝和移动构造/赋值函数，确保其在程序中的唯一性，以单例模式的思路进行管理。
 */
class RemoteManager : public RtmpPushManager
{
public:
    /**
     * @brief 析构函数。
     *
     * 负责调用 Close() 释放资源。
     */
    ~RemoteManager();

    /**
     * @brief 构造函数。
     *
     * 初始化成员变量，并创建一个事件循环（EventLoop）用于处理网络事件。
     */
    RemoteManager();

    // --- 禁止拷贝和移动，确保单例行为 ---
    RemoteManager(RemoteManager &&) = delete;
    RemoteManager(const RemoteManager &) = delete;
    RemoteManager &operator=(RemoteManager &&) = delete;
    RemoteManager &operator=(const RemoteManager &) = delete;

public:
    /**
     * @brief 初始化为“被控端”。
     *
     * 连接到信令服务器，并注册回调函数，准备好接收远程控制指令。
     * @param sigIp 信令服务器的 IP 地址。
     * @param port 信令服务器的端口。
     * @param code 本机的连接码，用于身份验证。
     */
    void Init(const QString& sigIp, uint16_t port, const QString& code);

    /**
     * @brief 启动“主控端”模式，发起远程连接。
     *
     * 创建并显示一个 PullerWgt（拉流窗口），并由 PullerWgt 内部逻辑处理后续的连接和拉流。
     * @param sigIp 信令服务器的 IP 地址。
     * @param port 信令服务器的端口。
     * @param code 要连接的对方的连接码。
     */
    void StartRemote(const QString& sigIp, uint16_t port, const QString& code);

protected:
    /**
     * @brief 处理停止推流指令的回调函数。
     *
     * 当作为被控端时，如果收到信令服务器的停止指令，此函数被调用。
     */
    void HandleStopStream();

    /**
     * @brief 处理开始推流指令的回调函数。
     *
     * 当作为被控端时，如果收到信令服务器的开始推流指令，此函数被调用。
     * @param streamAddr 服务器分配的用于推流的 RTMP 地址。
     * @return bool 成功开启推流返回 true，否则返回 false。
     */
    bool HandleStartStream(const QString& streamAddr);

private:
    /**
     * @brief 关闭并清理资源。
     *
     * 主要用于在程序退出时，停止推流（如果正在作为被控端）。
     */
    void Close();

private:
    std::unique_ptr<PullerWgt> pullerWgt_; ///< 主控端（拉流器）的UI窗口指针。
    std::unique_ptr<EventLoop> event_loop_; ///< 网络事件循环，为信令连接提供动力。
    std::shared_ptr<SigConnection> sig_conn_; ///< 信令连接对象，用于被控端与服务器的通信。
};

#endif // REMOTEMANAGER_H
