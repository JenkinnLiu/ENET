// 文件: BufferWriter.cpp
// 功能: 管理待发送数据包队列，支持分片发送、自动重试，并提供多种整数端序写入工具函数

// 引入必要头文件
#include "BufferWriter.h"
#include <string.h>     // memcpy
#include <unistd.h>     // close
#include <sys/socket.h> // send
#include <errno.h>      // errno 定义
#include <stdio.h>      // printf（可选）

// 构造函数：初始化发送队列的最大容量
// 参数：capacity - 最大允许缓存的数据包数量（默认为 kMaxQueueLength）
BufferWriter::BufferWriter(int capacity)
    : max_queue_length_(capacity)
{
    // 构造时可指定容量限制，防止队列无限增长导致内存耗尽
}

// Append(shared_ptr) 方法：
// 将已管理的缓冲区封装成 Packet 并入队，支持零拷贝
// 参数：
//   data  - 包含要发送数据的 shared_ptr<char>
//   size  - data 指向缓冲区的总长度
//   index - 从缓冲区中第 index 字节开始发送，默认为 0
// 返回：入队是否成功
bool BufferWriter::Append(std::shared_ptr<char> data, uint32_t size, uint32_t index)
{
    // 起始偏移不可超过数据总长度
    if (size <= index) return false;
    // 队列已满时拒绝入队
    if (buffer_.size() >= max_queue_length_) return false;
    // 构造 Packet 对象并入队
    Packet pkt = { data, size, index };
    buffer_.emplace(std::move(pkt));
    return true;
}

// Append(const char*) 方法：
// 接受裸指针数据，内部拷贝到新分配缓冲区后入队，保证原始数据生命周期安全
// 参数：同上
bool BufferWriter::Append(const char *data, uint32_t size, uint32_t index)
{
    // 参数合法性和队列容量检查
    if (size <= index) return false;
    if (buffer_.size() >= max_queue_length_) return false;

    // 分配新缓冲区，预留额外 512 字节以便追加协议头或扩展
    Packet pkt;
    pkt.data.reset(new char[size + 512], std::default_delete<char[]>());
    // 拷贝原始数据
    memcpy(pkt.data.get(), data, size);
    pkt.size = size;
    pkt.writeIndex = index;
    buffer_.emplace(std::move(pkt));
    return true;
}

// Send 方法：
// 从队列头部取出 Packet，调用 send 系统调用发送剩余数据
// 支持部分发送继续和重试机制
// 通过writeIndex 记录已发送偏移，直到整个 Packet 完全发送完毕后出队
// 参数：sockfd - 已连接的 socket 文件描述符
// 返回：
//   >0 - 本次实际发送的字节数
//    0 - 队列为空或遇到 EINTR/EAGAIN 需要重试
//   <0 - 发生其他发送错误
int BufferWriter::Send(int sockfd)
{
    int ret = 0;
    // count 控制最多处理当前包及紧接完成的下一个包，避免一次循环处理过多
    int count = 1;
    do {
        // 队列空，结束发送
        if (buffer_.empty()) return 0;
        count--;
        Packet &pkt = buffer_.front();
        // 从上次偏移处继续发送
        // 调用 send 发送数据, sockfd 是已连接的 socket 文件描述符, 从 pkt.writeIndex 开始发送
        // pkt.data.get() + pkt.writeIndex 是数据包剩余未发送部分的起始地址
        // pkt.size - pkt.writeIndex 是剩余未发送的字节数
        ret = ::send(sockfd, pkt.data.get() + pkt.writeIndex,
                     pkt.size - pkt.writeIndex, 0);
        if (ret > 0) {
            // 更新已发送偏移
            pkt.writeIndex += ret;
            // 数据包发送完毕，出队并允许再次发送下一个包
            if (pkt.writeIndex == pkt.size) {
                count++;
                buffer_.pop();
            }
        }
        else if (ret < 0) {
            // 对于中断和缓冲区暂无空间情况，不算错误，由调用者重试
            if (errno == EINTR || errno == EAGAIN) {
                ret = 0;
            }
        }
    } while (count > 0);
    return ret;
}

// 以下为整数按不同端序写入缓冲区的工具函数，方便协议编码

// 4 字节大端写入（高位在前）
void WriteUint32BE(char *p, uint32_t value)
{
    p[0] = (value >> 24) & 0xFF;
    p[1] = (value >> 16) & 0xFF;
    p[2] = (value >> 8) & 0xFF;
    p[3] = value & 0xFF;
}

// 4 字节小端写入（低位在前）
void WriteUint32LE(char *p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
    p[3] = (value >> 24) & 0xFF;
}

// 3 字节大端写入
void WriteUint24BE(char *p, uint32_t value)
{
    p[0] = (value >> 16) & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = value & 0xFF;
}

// 3 字节小端写入
void WriteUint24LE(char *p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
}

// 2 字节大端写入
void WriteUint16BE(char *p, uint32_t value)
{
    p[0] = (value >> 8) & 0xFF;
    p[1] = value & 0xFF;
}

// 2 字节小端写入
void WriteUint16LE(char *p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
}
