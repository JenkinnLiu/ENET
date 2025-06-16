#include "RtmpHandshake.h"
#include <random>
#include <string.h>

RtmpHandshake::RtmpHandshake(State state)
{
    handshake_state_ = state;
}

RtmpHandshake::~RtmpHandshake()
{
}

int RtmpHandshake::Parse(BufferReader &in_buffer, char *res_buf, uint32_t res_buf_size)
{
    uint8_t* buf = (uint8_t*)in_buffer.Peek();
    uint32_t buf_size = in_buffer.ReadableBytes();
    uint32_t pos = 0;
    uint32_t res_size = 0;
    std::random_device rd;

    if(handshake_state_ == HANDSHAKE_S0S1S2) //由客户端处理
    {
        if(buf_size < (1 + 1536 + 1536))
        {
            return res_size;
        }

        if(buf[0] != 3)
        {
            return -1;
        }

        pos += 1 + 1536 + 1536;
        res_size = 1536; //需要发送的数据大小
        //准备C2
        memcpy(res_buf,buf + 1,1536); //我们将这个S1传回去
        handshake_state_ = HANDSHAKE_COMPLETE;
    }
    else if(handshake_state_ == HANDSHAKE_C0C1) //由服务端处理
    {
        if(buf_size < 1 + 1536) //C0C1
        {
            return res_size;
        }
        else
        {
            if(buf[0] != 3)
            {
                return -1;
            }

            pos += 1537;
            res_size = 1 + 1536 + 1536;
            memset(res_buf,0,res_size); //返回S0S1S2
            res_buf[0] = 3;

            char*p = res_buf; p += 9;
            for(int i = 0;i < 1528; i++)
            {
                *p++ = rd();
            }
            memcpy(p,buf + 1,1536);
            handshake_state_ = HANDSHAKE_C2;
        }
    }
    else if(handshake_state_ == HANDSHAKE_C2)//服务器处理C2
    {
        if(buf_size < 1536) //C2不完整
        {
            return res_size;
        }
        else
        {
            pos += 1536;
            handshake_state_ = HANDSHAKE_COMPLETE;
        }
    }
    in_buffer.Retrieve(pos);
    return res_size;
}

int RtmpHandshake::BuildC0C1(char *buf, uint32_t buf_size) //客户端需要创建C0C1
{
    uint32_t size = 1 + 1536; //C0C1
    memset(buf,0,size);
    buf[0] = 3;//版本为3

    std::random_device rd;
    uint8_t* p = (uint8_t*)buf; p += 9;
    for(int i = 0;i < 1528;i++)
    {
        *p++ = rd();
    }
    return size;
}
