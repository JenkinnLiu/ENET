// 文件: BufferReader.cpp
// 功能: 管理接收缓冲区，支持动态扩容、数据读取以及按字节序解析整数的工具函数

// 引入必要头文件
#include "BufferReader.h"
#include <sys/socket.h> // recv 系统调用
#include <unistd.h>     // close

// 构造函数：初始化内部缓冲区大小
// 参数：initial_size - 初始缓冲区容量（字节数）
BufferReader::BufferReader(uint32_t initial_size)
{
    // 将内部缓冲区调整为 initial_size 大小
    buffer_.resize(initial_size);
}

// 析构函数：目前无需额外资源释放
BufferReader::~BufferReader()
{
    // no-op
}

// Read: 从 socket 文件描述符读取数据到内部缓冲区
// 步骤：
//  1. 检查剩余可写空间，若小于 MAX_BYTES_PER_READ，则尝试扩容一次
//     - 扩容后总容量不超过 MAX_BUFFER_SIZE
//  2. 调用 recv 将数据写入缓冲区末尾
//  3. 若读取成功，更新 writer_index_，返回实际读取字节数
// 参数：fd - 已连接的 socket 文件描述符
// 返回：>0 读取的字节数；0 表示暂无数据或已达最大缓存；<0 表示发生错误
int BufferReader::Read(int fd)
{
    //简单扩容
    uint32_t size = WritableBytes();// 获取当前可写空间大小
    if(size < MAX_BYTES_PER_READ)
    {
        uint32_t bufferReadSize = buffer_.size();
        if(bufferReadSize > MAX_BUFFER_SIZE)
        {
            return 0;
        }
        buffer_.resize(bufferReadSize + MAX_BYTES_PER_READ);
    }
    //读数据 从sock接收缓存 -> buffer
    // BeginWrite() 返回当前写索引位置的指针
    int bytes_read = ::recv(fd,BeginWrite(),MAX_BYTES_PER_READ,0);
    if(bytes_read > 0)
    {
        writer_index_ += bytes_read; // 更新写索引
    }
    return bytes_read;
}

// ReadAll: 将所有可读数据拷贝到字符串并重置读写索引
// 参数：data - 输出字符串，用于接收当前缓冲区中可读的数据
// 返回：拷贝到 data 的字节数
uint32_t BufferReader::ReadAll(std::string &data)
{
    uint32_t size = ReadableBytes();
    if(size > 0)
    {
        data.assign(Peek(),size); // 将可读数据拷贝到字符串
        // 重置读写索引
        writer_index_ = 0;
        reader_index_ = 0;
    }
    return size;
}

// 以下为按字节序解析整数的工具函数：

// ReadUint32BE: 从大端（Big Endian）字节流读取 uint32_t
uint32_t ReadUint32BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    return value;
}

// ReadUint32LE: 从小端（Little Endian）字节流读取 uint32_t
uint32_t ReadUint32LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

// ReadUint24BE: 从大端字节流读取 3 字节无符号整数
uint32_t ReadUint24BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
    return value;
}

// ReadUint24LE: 从小端字节流读取 3 字节无符号整数
uint32_t ReadUint24LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

// ReadUint16BE: 从大端字节流读取 uint16_t
uint16_t ReadUint16BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[0] << 8) | p[1];
    return value;
}

// ReadUint16LE: 从小端字节流读取 uint16_t
uint16_t ReadUint16LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[1] << 8) | p[0];
    return value;
}
