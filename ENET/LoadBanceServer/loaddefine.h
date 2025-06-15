/**
 * @file loaddefine.h
 * @brief LoadBalanceServer 协议定义，包含登录请求/应答及监控数据辅助类型
 *
 * 用于与 LoginServer 交互，获取可用后端服务的地址端口，并提供监控数据排序工具。
 */

#ifndef _LOADDEFINE_H_
#define _LOADDEFINE_H_
#include <cstdint>
#include <array>
#include "../LoginServer/define.h"


#pragma pack(push,1)

/**
 * @struct Login_Info
 * @brief 登录请求包体，继承自 packet_head，携带时间戳用于超时判断
 */
struct Login_Info : public packet_head
{
    Login_Info():packet_head()
    {
        cmd = Login;            ///< 登录命令标识
        len = sizeof(Login_Info);///< 包体总长度
        timestamp = -1;         ///< 客户端请求发送时间戳（毫秒）
    }
    uint64_t timestamp;       ///< 请求时间戳，用于服务端计算超时
};

/**
 * @struct LoginReply
 * @brief 登录应答包体，返回可用服务地址和端口
 */
struct LoginReply : public packet_head
{
    LoginReply():packet_head()
    {
        cmd = Login;             ///< 保持 Login 命令标识，超时或错误时置为 ERROR
        len = sizeof(LoginReply);///< 包体总长度
        port = -1;               ///< 分配的后端服务端口
        ip.fill('\0');          ///< 分配的后端服务 IP 字符串
    }
    uint16_t port;                ///< 后端服务端口
    std::array<char,16> ip;       ///< 后端服务 IPv4 地址字符串
};

/**
 * @typedef MinotorPair
 * @brief 监控数据对，first: 服务索引，second: 监控数据指针
 */
typedef std::pair<int, Monitor_body*> MinotorPair;

/**
 * @struct CmpByValue
 * @brief 监控数据比较器，根据 mem 字段从小到大排序
 */
struct CmpByValue
{
    bool operator()(const MinotorPair& l, const MinotorPair& r) const
    {
        return l.second->mem < r.second->mem; ///< 比较内存使用量
    }
};

#pragma pack(pop)

#endif //_LOADDEFINE_H_