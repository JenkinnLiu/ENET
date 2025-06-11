//=============================================================================
// 文件: RtmpChunk.h
// 描述: 定义RTMP协议中的chunk解析和构建类(RtmpChunk)，
//       负责将网络接收到的chunk数据拼装为完整消息(RtmpMessage)，
//       并将完整消息拆分为符合chunk大小的chunk序列。
//       区分chunk流(chunk stream)和消息流(message stream)的概念：
//       - chunk stream: 实际传输分片，按csid区分
//       - message stream: 完整业务消息，由多个chunk组装
//=============================================================================

#include "../EdoyunNet/BufferReader.h"
#include "../EdoyunNet/BufferWriter.h"
#include "RtmpMessage.h"
#include <map>

class RtmpChunk
{
public:
    // 当前解析状态：先解析header再解析body
    enum State
    {
        PARSE_HEADER,  // 解析chunk头部
        PARSE_BODY     // 解析chunk数据体
    };

    // 构造：初始化状态并为新的chunk stream分配csid
    RtmpChunk();
    virtual ~RtmpChunk();

    // Parse: 从in_buffer读取chunk数据，填充out_rtmp_msg
    // 返回>=0: 本次解析消耗的字节数，0表示需要更多数据，<0表示出错
    int Parse(BufferReader& in_buffer, RtmpMessage& out_rtmp_msg);

    // CreateChunk: 将完整消息in_msg拆分并写入buf，buf_size为缓冲区大小
    // csid为输出chunk stream id，返回写入总字节数，失败返回-1
    int CreateChunk(uint32_t csid, RtmpMessage& in_msg, char* buf, uint32_t buf_size);

    // 设置接收端chunk大小
    void SetInChunkSize(uint32_t in_chunk_size)
    {
        in_chunk_size_ = in_chunk_size;
    }
    // 设置发送端chunk大小
    void SetOutChunkSize(uint32_t out_chunk_size)
    {
        out_chunk_size_ = out_chunk_size;
    }

    // 清除内部缓存的未完成消息
    void Clear()
    {
        rtmp_message_.clear();
    }

    // 获取当前活动的chunk stream id
    int GetStreamId() const
    {
        return stream_id_;
    }

protected:
    // 解析chunk的基本报头和消息报头，准备后续body解析
    int ParseChunkHeader(BufferReader& buffer);
    // 解析chunk的数据体，将payload拼装到对应RtmpMessage
    int ParseChunkBody(BufferReader& buffer);
    // 构建Basic Header字段，fmt和csid映射到一到三字节
    int CreateBasicHeader(uint8_t fmt, uint32_t csid, char* buf);
    // 构建Message Header，根据fmt写入时间戳、长度、类型和stream id
    int CreateMessageHeader(uint8_t fmt, RtmpMessage& rtmp_msg, char* buf);

private:
    State state_;                      // 当前解析状态
    int chunk_stream_id_ = -1;         // 当前解析的chunk stream id
    static int stream_id_;             // 静态自增，用于分配唯一chunk stream id
    uint32_t in_chunk_size_ = 128;     // 最大接收chunk大小（字节）
    uint32_t out_chunk_size_ = 128;    // 发送chunk大小
    std::map<int, RtmpMessage> rtmp_message_;  // 按csid存储未完成消息
    const int KChunkMessageHeaderLenght[4] = {11,7,3,0}; // 不同fmt下message header长度
};