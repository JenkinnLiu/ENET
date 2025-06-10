// 文件: RtmpHandshake.h
// 功能: 定义 RTMP 握手协议的状态机及接口，支持客户端和服务端握手消息的构造与解析
// RTMP 握手流程共六个消息：
//   客户端发送 C0(version)
//   客户端发送 C1(time + zero + random)
//   服务端发送 S0(version) + S1(time + zero + random) + S2(time2 + random echo)
//   客户端发送 C2(time2 + random echo)

#include "../EdoyunNet/BufferReader.h"

class RtmpHandshake
{
public:
    // 握手状态枚举
    enum State
    {
        HANDSHAKE_C0C1,      // 服务端接收客户端的 C0 和 C1
        HANDSHAKE_S0S1S2,    // 客户端接收并解析服务端的 S0 S1 S2
        HANDSHAKE_C2,        // 服务端接收客户端的 C2
        HANDSHAKE_COMPLETE   // 握手完成
    };

    // 构造函数：传入初始状态（客户端或服务端）
    explicit RtmpHandshake(State state);
    virtual ~RtmpHandshake();

    // Parse: 解析并处理输入缓冲区中的握手消息
    // @param in_buffer    输入缓存区，包含对端发送的握手数据
    // @param res_buf      输出缓冲区，用于存放需要发送回对端的响应数据
    // @param res_buf_size res_buf 的最大容量
    // @return 生成的响应数据长度，0 表示尚未收到完整消息，-1 表示协议版本错误或非法
    int Parse(BufferReader& in_buffer, char* res_buf, uint32_t res_buf_size);

    // 判断握手是否完成
    bool IsCompleted() const { return handshake_state_ == HANDSHAKE_COMPLETE; }
    
    // BuildC0C1: 客户端调用，构造 C0(version) + C1(time + zero + random)
    // @param buf       输出缓冲区，用于存放 C0C1
    // @param buf_size  缓冲区大小，应 >= 1537
    // @return 写入的字节数（1537）
    int BuildC0C1(char* buf, uint32_t buf_size);

private:
    State handshake_state_;  // 当前握手状态
};