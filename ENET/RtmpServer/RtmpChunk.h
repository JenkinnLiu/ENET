#include "../EdoyunNet/BufferReader.h"
#include "../EdoyunNet/BufferWriter.h"
#include "RtmpMessage.h"
#include <map>

class RtmpChunk
{
public:
    enum State
    {
        PARSE_HEADER,
        PARSE_BODY,
    };
    RtmpChunk();
    virtual ~RtmpChunk();
    int Parse(BufferReader& in_buffer,RtmpMessage& out_rtmp_msg);
    int CreateChunk(uint32_t csid,RtmpMessage& in_msg,char* buf,uint32_t buf_size);
    void SetInChunkSize(uint32_t in_chunk_size)
    {
        in_chunk_size_ = in_chunk_size;
    }
    void SetOutChunkSize(uint32_t out_chunk_size)
    {
        out_chunk_size_ = out_chunk_size;
    }
    void Clear()
    {
        rtmp_message_.clear();
    }
    int GetStreamId()const{
        return stream_id_;
    }
protected:
    int ParseChunkHeader(BufferReader& buffer);
    int ParseChunkBody(BufferReader& buffer);
    int CreateBasicHeader(uint8_t fmt,uint32_t csid,char* buf);
    int CreateMessageHeader(uint8_t fmt,RtmpMessage& rtmp_msg,char* buf);
private:
    State state_;
    int chunk_stream_id_ = 0;
    static int stream_id_;
    uint32_t in_chunk_size_ = 128;
    uint32_t out_chunk_size_ = 128;
    std::map<int,RtmpMessage> rtmp_message_;
    const int KChunkMessageHeaderLenght[4] = {11,7,3,0};
};