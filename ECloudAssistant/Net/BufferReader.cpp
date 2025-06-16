#include "BufferReader.h"
#include "TcpSocket.h"

BufferReader::BufferReader(uint32_t initial_size)
{
    buffer_.resize(initial_size);
}

BufferReader::~BufferReader()
{
}

int BufferReader::Read(int fd)
{
    //简单扩容
    uint32_t size = WritableBytes();
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
    int bytes_read = ::recv(fd,BeginWrite(),MAX_BYTES_PER_READ,0);
    if(bytes_read > 0)
    {
        writer_index_ += bytes_read;
    }
    return bytes_read;
}

uint32_t BufferReader::ReadAll(std::string &data)
{
    uint32_t size = ReadableBytes();
    if(size > 0)
    {
        data.assign(Peek(),size);
        writer_index_ = 0;
        reader_index_ = 0;
    }
    return size;
}

uint32_t ReadUint32BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    return value;
}

uint32_t ReadUint32LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

uint32_t ReadUint24BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
    return value;
}

uint32_t ReadUint24LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

uint16_t ReadUint16BE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[0] << 8) | p[1];
    return value;
}

uint16_t ReadUint16LE(char *data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[1] << 8) | p[0];
    return value;
}
