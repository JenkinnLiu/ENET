//=============================================================================
// 文件: RtmpChunk.cpp
// 描述: RtmpChunk实现文件，包含chunk解析和构建的详细逻辑。
//    解析流程: Basic Header -> Message Header -> Extended Timestamp -> Chunk Data
//    构建流程: Message拆分为多个chunk，分别写入Basic/Header、Extended TS、Chunk Data
//=============================================================================

// fmt代表RTMP Chunk 的“格式类型”（chunk format），取值范围 0–3，用来指示后续 Message Header 的长度和内容：

// fmt=0：完整的 Message Header（11 字节）
// fmt=1：省略流 ID 的 Message Header（7 字节）
// fmt=2：只保留时间增量的 Message Header（3 字节）
// fmt=3：无 Message Header（0 字节），完全复用上一个 chunk 的信息
#include "RtmpChunk.h"
#include <string.h>

int RtmpChunk::stream_id_ = 0;

RtmpChunk::RtmpChunk()
{
    // 初始化解析状态并分配唯一chunk stream id
    state_ = PARSE_HEADER;
    chunk_stream_id_ = -1;
    ++stream_id_;
}

RtmpChunk::~RtmpChunk()
{
}

int RtmpChunk::Parse(BufferReader &in_buffer, RtmpMessage &out_rtmp_msg)
{
    int ret = 0;
    // 当buffer中无数据时，直接返回0等待更多数据
    if (!in_buffer.ReadableBytes())
    {
        return 0;
    }

    if (state_ == PARSE_HEADER)
    {
        // 先解析chunk报头
        ret = ParseChunkHeader(in_buffer);
    }
    else
    {
        // 解析chunk数据体
        ret = ParseChunkBody(in_buffer);
        // 解析成功且已累计到完整消息时
        if (ret > 0 && chunk_stream_id_ >= 0)
        {
            auto& rtmp_msg = rtmp_message_[chunk_stream_id_];
            // 判断payload是否填充完成
            if (rtmp_msg.index == rtmp_msg.lenght)
            {
                // 更新绝对时间戳(_timestamp)
                if (rtmp_msg.timestamp >= 0xFFFFFF)
                {
                    rtmp_msg._timestamp += rtmp_msg.extend_timestamp;
                }
                else
                {
                    rtmp_msg._timestamp += rtmp_msg.timestamp;
                }
                // 将完整消息输出
                out_rtmp_msg = rtmp_msg;
                // 重置状态，释放message缓冲
                chunk_stream_id_ = -1;
                rtmp_msg.Clear();
            }
        }
    }
    return ret;
}

// CreateChunk: 将完整消息in_msg拆分并写入buf，buf_size为缓冲区大小
// csid为输出chunk stream id，返回写入总字节数，失败返回-1
int RtmpChunk::CreateChunk(uint32_t csid, RtmpMessage &in_msg, char *buf, uint32_t buf_size)
{
    uint32_t buf_offset = 0;
    uint32_t playload_offset = 0;
    // 预估最坏情况需要的缓冲区(capacity) = payload + 每个chunk可能4字节扩展ts + Basic头
    uint32_t capacity = in_msg.lenght + in_msg.lenght / out_chunk_size_ * 5;
    if (buf_size < capacity)
    {
        return -1;
    }
    // 写入第一个chunk的Basic Header和Message Header
    buf_offset += CreateBasicHeader(0, csid, buf + buf_offset);
    buf_offset += CreateMessageHeader(0, in_msg, buf + buf_offset);
    // 写入扩展时间戳（当_timestamp>=0xFFFFFF时）
    if (in_msg._timestamp >= 0xFFFFFF)
    {
        WriteUint32BE((char*)buf + buf_offset, (uint32_t)in_msg.extend_timestamp);
        buf_offset += 4;
    }
    // 拆分payload到多个chunk
    while (in_msg.lenght > 0)
    {
        if (in_msg.lenght > out_chunk_size_)
        {
            // 写入chunk_size长度的数据
            memcpy(buf + buf_offset, in_msg.playload.get() + playload_offset, out_chunk_size_);
            playload_offset += out_chunk_size_;
            buf_offset += out_chunk_size_;
            in_msg.lenght -= out_chunk_size_;
            // 写入后续chunk的Basic Header(FMT=3)和可选扩展TS
            buf_offset += CreateBasicHeader(3, csid, buf + buf_offset);
            if (in_msg._timestamp >= 0xFFFFFF)
            {
                WriteUint32BE(buf + buf_offset, (uint32_t)in_msg.extend_timestamp);
                buf_offset += 4;
            }
        }
        else
        {
            // 写入最后一个包的剩余payload
            memcpy(buf + buf_offset, in_msg.playload.get() + playload_offset, in_msg.lenght);
            buf_offset += in_msg.lenght;
            in_msg.lenght = 0;
            break;
        }
    }
    return buf_offset;
}

// 解析chunk的基本报头和消息报头，准备后续body解析
int RtmpChunk::ParseChunkHeader(BufferReader &buffer)
{
    uint32_t bytes_used = 0;// 已解析的字节数
    // 获取当前缓冲区的起始指针和可读字节数
    uint8_t* buf = (uint8_t*)buffer.Peek();
    uint32_t buf_size = buffer.ReadableBytes();

    // 1. 解析Basic Header(fmt一共有3个版本): 读取fmt和csid
    uint8_t flags = buf[bytes_used];
    
    uint8_t fmt = (flags >> 6); // 高两位存放fmt: 0-3
    if (fmt >= 4)
    {
        return -1; // FMT范围0-3
    }
    bytes_used += 1;
    uint8_t csid = flags & 0x3F; // 低六位存放CSID
    if (csid == 0) 
    {
        // 1字节形式: csid = 0
        if (buf_size < bytes_used) return 0; // 数据不足
        csid = 0; // CSID为0表示使用默认chunk stream
    }
    else if (csid < 64)
    {
        // 1字节形式: csid = buf[0] & 0x3F
        // 已经在flags中解析过了
    }
    else if (csid < 320)
    {
        // 2字节形式: csid = buf[1] + 64
        if (buf_size < (bytes_used + 1)) return 0;
        csid = buf[bytes_used] + 64;
        bytes_used += 1;
    }
    else if (csid == 1)
    {
        // 3字节形式: csid = buf[1] + buf[2]*256 + 64
        if (buf_size < (bytes_used + 2)) return 0;
        csid = buf[bytes_used] + buf[bytes_used+1] * 256 + 64;
        bytes_used += 2;
    }

    // 2. 解析Message Header（共有4个版本）: 长度根据fmt从11,7,3,0字节不等
    uint32_t header_len = KChunkMessageHeaderLenght[fmt];
    if (buf_size < (bytes_used + header_len)) return 0;
    RtmpMessageHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(&header, buf + bytes_used, header_len);
    bytes_used += header_len;

    auto& rtmp_msg = rtmp_message_[csid];
    chunk_stream_id_ = rtmp_msg.csid = csid;

    // 在FMT<=1时，需要(重新)分配payload缓冲区和设置消息类型
    if (fmt <= 1)
    {
        uint32_t lenght = ReadUint24BE((char*)header.lenght);
        if (rtmp_msg.lenght != lenght || !rtmp_msg.playload)
        {
            rtmp_msg.lenght = lenght;
            rtmp_msg.playload.reset(new char[lenght], std::default_delete<char[]>());
        }
        rtmp_msg.index = 0;
        rtmp_msg.type_id = header.type_id;
    }

    // 在FMT=0时，额外设置message stream id
    if (fmt == 0)
    {
        rtmp_msg.stream_id = ReadUint32LE((char*)header.stream_id);
    }

    // 3. 解析或累加时间戳
    uint32_t timestamp = ReadUint24BE((char*)header.timestamp);
    uint32_t extend_ts = 0;
    if (timestamp >= 0xFFFFFF || rtmp_msg.timestamp >= 0xFFFFFF)
    {
        // 读取扩展时间戳(4字节)
        if (buf_size < (bytes_used + 4)) return 0;
        extend_ts = ReadUint32BE((char*)buf + bytes_used);
        bytes_used += 4;
    }
    // 根据fmt将timestamp累加到rtmp_msg
    if (rtmp_msg.index == 0)
    {
        // 第一个chunk: 初始化timestamp和extend_timestamp
        rtmp_msg._timestamp = 0;
        rtmp_msg.timestamp = timestamp;
        rtmp_msg.extend_timestamp = extend_ts;
    }
    else
    {
        // 后续chunk: 累加timestamp或extend_timestamp
        if (rtmp_msg.timestamp >= 0xFFFFFF)
            rtmp_msg.extend_timestamp += extend_ts;
        else
            rtmp_msg.timestamp += timestamp;
    }

    // 准备解析Body
    state_ = PARSE_BODY;
    buffer.Retrieve(bytes_used);
    return bytes_used;
}

// 解析chunk的数据体，将payload拼装到对应RtmpMessage
int RtmpChunk::ParseChunkBody(BufferReader &buffer)
{
    uint32_t bytes_used = 0;
    uint8_t* buf = (uint8_t*)buffer.Peek();
    uint32_t buf_size = buffer.ReadableBytes();

    if (chunk_stream_id_ < 0)
    {
        return -1; // header未正确解析
    }

    auto& rtmp_msg = rtmp_message_[chunk_stream_id_];
    // 计算当前chunk应读取多少payload
    uint32_t to_copy = rtmp_msg.lenght - rtmp_msg.index;
    if (to_copy > in_chunk_size_) to_copy = in_chunk_size_;
    if (buf_size < to_copy) return 0; // 数据不足

    // 拷贝payload数据, 从当前chunk的index位置开始
    memcpy(rtmp_msg.playload.get() + rtmp_msg.index, buf + bytes_used, to_copy);
    bytes_used += to_copy;
    rtmp_msg.index += to_copy;

    // 若读取完本chunk或达完整消息边界，则切回header解析
    if (rtmp_msg.index >= rtmp_msg.lenght || rtmp_msg.index % in_chunk_size_ == 0)
    {
        state_ = PARSE_HEADER;
    }
    buffer.Retrieve(bytes_used);// 更新缓冲区已读字节数
    // 返回本次解析消耗的字节数
    return bytes_used;
}

int RtmpChunk::CreateBasicHeader(uint8_t fmt, uint32_t csid, char *buf)
{
    int len = 0;
    // CSID<64: 单字节Basic Header
    if (csid < 64)
    {
        buf[len++] = (fmt << 6) | csid;
    }
    else if (csid < 64 + 256)
    {
        // CSID>=64且<320: 两字节Basic Header
        buf[len++] = (fmt << 6) | 0;
        buf[len++] = (csid - 64) & 0xFF;
    }
    else
    {
        // CSID>=320: 三字节Basic Header
        buf[len++] = (fmt << 6) | 1;
        buf[len++] = (csid - 64) & 0xFF;
        buf[len++] = ((csid - 64) >> 8) & 0xFF;
    }
    return len;
}

int RtmpChunk::CreateMessageHeader(uint8_t fmt, RtmpMessage &rtmp_msg, char *buf)
{
    int len = 0;
    // fmt<=2: 写入时间戳(3字节)
    if (fmt <= 2)
    {
        if (rtmp_msg._timestamp < 0xFFFFFF)
        {
            WriteUint24BE(buf, (uint32_t)rtmp_msg._timestamp);
        }
        else
        {
            // 超过0xFFFFFF则写0xFFFFFF，具体值放扩展TS
            WriteUint24BE(buf, 0xFFFFFF);
        }
        len += 3;
    }
    // fmt<=1: 写入消息长度和类型
    if (fmt <= 1)
    {
        WriteUint24BE(buf + len, rtmp_msg.lenght);
        len += 3;
        buf[len++] = rtmp_msg.type_id;
    }
    // fmt==0: 写入消息流ID(4字节，小端)
    if (fmt == 0)
    {
        WriteUint32LE(buf + len, rtmp_msg.stream_id);
        len += 4;
    }
    return len;
}
