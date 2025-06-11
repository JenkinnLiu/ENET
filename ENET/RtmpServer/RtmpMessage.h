//=============================================================================
// 文件: RtmpMessage.h
// 描述: 定义RTMP消息的头部(RtmpMessageHeader)和消息体(RtmpMessage)结构，
//       解释了RTMP消息与分片(chunk)的关系，以及如何在内存中组织完整RTMP消息。
//       包含时间戳扩展、消息长度、消息类型及流ID的小端/大端字节序说明。
//=============================================================================

#include <cstdint>
#include <memory>

// RtmpMessageHeader: 源自RTMP协议的消息头基本字段
//  - timestamp(3字节, 大端): 消息的相对时间戳或增量时间戳
//  - lenght(3字节, 大端): 消息体(payload)的长度
//  - type_id(1字节): 表示RTMP消息类型，如音视频数据、控制消息等
//  - stream_id(4字节, 小端): 表示消息所属的流ID，用于多路复用
struct RtmpMessageHeader
{
    uint8_t timestamp[3];
    uint8_t lenght[3];
    uint8_t type_id;
    uint8_t stream_id[4]; // 小端存储
};

// RtmpMessage: 表示完整RTMP消息的结构体
//  - timestamp: 收到或发送时头部的24位时间戳
//  - lenght: 消息体总长度
//  - type_id: RTMP消息类型标识
//  - stream_id: RTMP流ID
//  - extend_timestamp: 当timestamp>=0xFFFFFF时的32位扩展时间戳
//  - _timestamp: 绝对时间戳(累加timestamp或extend_timestamp获得)
//  - codeId: 应用层自定义的编码或标识ID
//  - csid: 关联的chunk stream ID，用于将多个chunk拼装为同一消息
//  - index: 当前已填充payload数据的字节偏移量
//  - playload: 存放消息体数据的缓冲区，用shared_ptr管理
struct RtmpMessage
{
    uint32_t timestamp = 0;
    uint32_t lenght = 0;
    uint8_t type_id = 0;
    uint32_t stream_id = 0;
    uint32_t extend_timestamp = 0;

    uint64_t _timestamp = 0;
    uint8_t codeId = 0;

    uint8_t csid = 0;
    uint32_t index = 0;
    std::shared_ptr<char> playload = nullptr;

    // Clear(): 重置消息状态并根据length重新分配payload缓冲区
    //  调用前请确保lenght已正确设置
    void Clear()
    {
        index = 0;
        timestamp = 0;
        extend_timestamp = 0;
        if (lenght > 0)
        {
            playload.reset(new char[lenght], std::default_delete<char[]>());
        }
    }

    // IsCompleted(): 判断消息payload是否已填充完毕
    //  当index等于lenght且playload已分配时返回true
    bool IsCompleted() const
    {
        if (index == lenght && lenght > 0 && playload != nullptr)
        {
            return true;
        }
        return false;
    }
};
