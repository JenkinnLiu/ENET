// 文件: RtmpHandshake.cpp
// 功能: 实现 RTMP 握手协议的解析和构造
// RTMP 握手顺序：
//   客户端 -> 服务端: C0（版本号 1 字节）
//   客户端 -> 服务端: C1（4 字节 time + 4 字节 zero + 1528 字节 random）
//   服务端 -> 客户端: S0（版本号 1 字节）
//   服务端 -> 客户端: S1（4 字节 time + 4 字节 zero + 1528 字节 random）
//   服务端 -> 客户端: S2（4 字节 time2 + 4 字节 time from C1 + 1528 字节 random echo）
//   客户端 -> 服务端: C2（4 字节 time2 + 4 字节 time from S1 + 1528 字节 random echo）

#include "RtmpHandshake.h"
#include <random>
#include <string.h>

// 构造函数：设置初始握手状态（服务端接收 C0C1 或客户端接收 S0S1S2）
RtmpHandshake::RtmpHandshake(State state)
{
    handshake_state_ = state;
}

RtmpHandshake::~RtmpHandshake()
{
}

// Parse: 根据当前状态解析输入缓冲区中的握手数据，并生成响应数据到 res_buf
// @param in_buffer    包含对端发送的握手原始数据
// @param res_buf      用于存放需发送回对端的握手响应
// @param res_buf_size res_buf 的最大可用大小
// @return 响应数据字节数 (>0)，0 表示数据不完整，-1 表示协议错误
int RtmpHandshake::Parse(BufferReader &in_buffer, char *res_buf, uint32_t res_buf_size)
{
    uint8_t* buf = (uint8_t*)in_buffer.Peek();             // 获取输入缓冲
    uint32_t buf_size = in_buffer.ReadableBytes();        // 可读字节数
    uint32_t pos = 0;                                     // 解析位置指针
    uint32_t res_size = 0;                                // 本次需要发送的响应长度
    std::random_device rd;                                // 随机生成器

    if (handshake_state_ == HANDSHAKE_S0S1S2) // 客户端接收 S0+S1+S2
    {
        // 需要至少 1 + 1536 + 1536 字节才能完整解析 S0/S1/S2
        if (buf_size < (1 + 1536 + 1536))
        {
            return res_size;  // 数据不完整，等待更多字节
        }
        // 检查版本号 S0，必须为 3
        if (buf[0] != 3)
        {
            return -1;       // 协议版本不匹配
        }
        // 跳过 S0(1字节)+S1(1536)+S2(1536)
        pos += 1 + 1536 + 1536;
        // 准备 C2：回显 S1 的随机数部分（1536 字节）
        res_size = 1536;
        memcpy(res_buf, buf + 1, 1536);
        handshake_state_ = HANDSHAKE_COMPLETE;
    }
    else if (handshake_state_ == HANDSHAKE_C0C1) // 服务端接收 C0+C1
    {
        // 需要至少 1 + 1536 字节才能解析 C0C1
        if (buf_size < 1 + 1536)
        {
            return res_size;
        }
        // 检查版本号 C0，必须为 3
        if (buf[0] != 3)
        {
            return -1;
        }
        pos += 1 + 1536;
        // 准备 S0+S1+S2，总长度 = 1 + 1536 + 1536
        res_size = 1 + 1536 + 1536;
        // 清零缓冲并设置 S0 版本号
        memset(res_buf, 0, res_size);
        res_buf[0] = 3;
        // 构造 S1: at offset 1, time + zero + random
        char *p = res_buf + 1 + 4 + 4;  // 跳过 time(4) + zero(4)
        for (int i = 0; i < 1528; ++i)
        {
            *p++ = rd();  // 随机数保证唯一性
        }
        // 构造 S2: 回显 C1 的 1536 字节随机数
        memcpy(p, buf + 1, 1536);
        handshake_state_ = HANDSHAKE_C2;
    }
    else if (handshake_state_ == HANDSHAKE_C2) // 服务端接收客户端的 C2
    {
        // C2 长度为 1536
        if (buf_size < 1536)
        {
            return res_size;
        }
        pos += 1536;       // 跳过 C2
        handshake_state_ = HANDSHAKE_COMPLETE;
    }
    // 从输入缓冲区消费已解析的字节
    in_buffer.Retrieve(pos);
    return res_size;
}

// BuildC0C1: 客户端调用，构造初始握手消息 C0 + C1
// @param buf      输出缓冲区，长度应 >= 1537
// @param buf_size 缓冲区大小
// @return 写入的字节数（1 + 1536）
int RtmpHandshake::BuildC0C1(char *buf, uint32_t buf_size)
{
    uint32_t size = 1 + 1536;
    memset(buf, 0, size);
    buf[0] = 3;  // C0 版本号
    // 构造 C1: 时间戳(4字节)=0、zero(4字节)=0、random(1528字节)
    std::random_device rd;
    uint8_t *p = (uint8_t*)buf + 1 + 4 + 4;
    for (int i = 0; i < 1528; ++i)
    {
        *p++ = rd();
    }
    return size;
}
