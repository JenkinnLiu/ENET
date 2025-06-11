#include "amf.h"
#include "../EdoyunNet/BufferReader.h"
#include "../EdoyunNet/BufferWriter.h"
#include <string.h>

int AmfDecoder::decode(const char *data, int size, int n)
{
    int bytes_used = 0;
    while (bytes_used < size)
    {
        int ret = 0;
        char type = data[bytes_used];
        bytes_used += 1;

        switch (type)
        {
        case AMF0_NUMBER:
            m_obj.type = AMF_NUMBER;
            ret = decodeNumber(data + bytes_used,size - bytes_used,m_obj.amf_number);
            break;
        case AMF0_BOOLEAN:
            m_obj.type = AMF_BOOLEAN;
            ret = decodeBoolean(data + bytes_used,size - bytes_used,m_obj.amf_boolean);
            break;
        case AMF0_STRING:
            m_obj.type = AMF_STRING;
            ret = decodeString(data + bytes_used,size - bytes_used,m_obj.amf_string);
            break;
        case AMF0_OBJECT:
            ret = decodeObject(data + bytes_used,size - bytes_used,m_objs);
            break;
        case AMF0_ECMA_ARRAY:
            ret = decodeObject(data + bytes_used + 4,size - bytes_used - 4,m_objs);
        default:
            break;
        }

        if(ret < 0)
        {
            break;
        }

        bytes_used += ret;
        n--;
        if(n == 0)
        {
            break;
        }
    }

    return bytes_used;
}

int AmfDecoder::decodeBoolean(const char *data, int size, bool &amf_boolean)
{
    if(size < 1)
    {
        return 0;
    }

    amf_boolean = (data[0] != 0);
    return 1;
}

int AmfDecoder::decodeNumber(const char *data, int size, double &amf_number)
{
    if(size < 8) //double 8
    {
        return 0;
    }

    char* ci = (char*)data;
    char* co = (char*)&amf_number;
    //这是大小端转换
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];

    return 8;
}

int AmfDecoder::decodeString(const char *data, int size, std::string &amf_string)
{
    if(size < 2) //string长度 2字节表示
    {
        return 0;
    }

    int bytes_used = 0;
    //获取大小
    int strSize = decodeInt16(data,size);
    bytes_used += 2;
    //获取string
    if(strSize > (size - bytes_used))//说明数据不全
    {
        return -1;
    }

    amf_string = std::string(&data[bytes_used],0,strSize);
    bytes_used += strSize;
    return bytes_used;
}

int AmfDecoder::decodeObject(const char *data, int size, AmfObjects &amf_objs)
{
    amf_objs.clear();
    int bytes_used = 0;
    while(size > 0)
    {
        int strLen = decodeInt16(data + bytes_used,size); //获取字符串长度
        size -= 2;
        if(size < strLen)//说明这个数据不完整
        {
            return bytes_used;
        }

        //获取键
        std::string key(data + bytes_used + 2,0,strLen);
        size -= strLen;

        //获取对象
        //先来创建解码对象
        AmfDecoder dec;
        int ret = dec.decode(data + bytes_used + 2 + strLen,size,1); //每次解码一个对象 返回值就是消耗多少字节数
        bytes_used += 2 + strLen + ret;
        if(ret <= 1)
        {
            break;
        }
        //插入键值
        amf_objs.emplace(key,dec.getObject());
    }
    return bytes_used;
}

uint16_t AmfDecoder::decodeInt16(const char *data, int size)
{
    return ReadUint16BE((char*)data);
}

uint32_t AmfDecoder::decodeInt24(const char *data, int size)
{
    return ReadUint24BE((char*)data);
}

uint32_t AmfDecoder::decodeInt32(const char *data, int size)
{
    return ReadUint32BE((char*)data);
}

AmfEncoder::AmfEncoder(uint32_t size)
    :m_data(new char[size],std::default_delete<char[]>())
    ,m_size(size)
{
}

AmfEncoder::~AmfEncoder()
{
}

void AmfEncoder::encodeString(const char *str, int len, bool isObject)
{
    //编码字符串
    if((m_size - m_index) < (len + 1 + 2 + 2)) //当前内存剩余空间不足
    {
        this->realloc(m_size + len + 5); //扩容
    }

    if(len < 65536) //afm0_string
    {
        if(isObject)
        {
            m_data.get()[m_index++] = AMF0_STRING;  //将类型赋值
        }
        encodeInt16(len);//编码长度  1字节类型 + 2长度 + 具体数值(string numnber bool)
    }
    else //amf0_long_string
    {
        if(isObject)
        {
            m_data.get()[m_index++] = AMF0_LONG_STRING;
        }
        encodeInt32(len);
    }

    memcpy(m_data.get() + m_index,str,len);
    m_index += len;
}

void AmfEncoder::encodeNumber(double value)
{
    if((m_size - m_index) < 9)  //1字节类型 8字节double
    {
        this->realloc(m_size + 1024);
    }

    m_data.get()[m_index++] = AMF0_NUMBER;

    //写入数据
    char* ci = (char*)&value;
    char* co = m_data.get();
    //再去赋值  做大小端转换
    co[m_index++] = ci[7];
    co[m_index++] = ci[6];
    co[m_index++] = ci[5];
    co[m_index++] = ci[4];
    co[m_index++] = ci[3];
    co[m_index++] = ci[2];
    co[m_index++] = ci[1];
    co[m_index++] = ci[0];
}

void AmfEncoder::encodeBoolean(int value)
{
    if((m_size - m_index) < 2) //1字节类型 bool 1字节
    {
        this->realloc(m_size + 1024);
    }

    m_data.get()[m_index++] = AMF0_BOOLEAN;
    m_data.get()[m_index++] = value ? 0x01 : 0x00;
}

void AmfEncoder::encodeObjects(AmfObjects &objs)
{
    if(objs.size() == 0) //对象为空
    {
        encodeInt8(AMF0_NULL);
        return ;
    }

    //否则不为空，就是一个对象类型
    
    encodeInt8(AMF0_OBJECT);
    for(auto iter : objs) //遍历编码 键-> string 值->对象
    {
        //我们先编码字符 编码键
        encodeString(iter.first.c_str(),(int)iter.first.size(),false);
        //编码值
        switch (iter.second.type)
        {
        case AMF_NUMBER:
            encodeNumber(iter.second.amf_number);
            break;
        case AMF_STRING:
            encodeString(iter.second.amf_string.c_str(),(int)iter.second.amf_string.size());
            break;
        case AMF_BOOLEAN:
            encodeBoolean(iter.second.amf_boolean);
            break;
        default:
            break;
        }
    }

    //结尾
    encodeString("",0,false);
    encodeInt8(AMF0_OBJECT_END);
}

void AmfEncoder::encodeECMA(AmfObjects &objs)
{
    //编码对象数组
    encodeInt8(AMF0_ECMA_ARRAY);
    encodeInt32(0);

    //编码对象数组
    for(auto iter : objs)
    {
         //我们先编码字符 编码键
        encodeString(iter.first.c_str(),(int)iter.first.size(),false);
        //编码值
        switch (iter.second.type)
        {
        case AMF_NUMBER:
            encodeNumber(iter.second.amf_number);
            break;
        case AMF_STRING:
            encodeString(iter.second.amf_string.c_str(),(int)iter.second.amf_string.size());
            break;
        case AMF_BOOLEAN:
            encodeBoolean(iter.second.amf_boolean);
            break;
        default:
            break;
        }
    }
    
    //结尾
    encodeString("",0,false);
    encodeInt8(AMF0_OBJECT_END);
}

void AmfEncoder::encodeInt8(int8_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    m_data.get()[m_index++] = value;
}

void AmfEncoder::encodeInt16(int16_t value)
{
    if((m_size - m_index) < 2)
    {
        this->realloc(m_size + 1024);
    }

    WriteUint16BE(m_data.get() + m_index,value);
    m_index += 2;
}

void AmfEncoder::encodeInt24(int32_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    
    WriteUint24BE(m_data.get() + m_index,value);
    m_index += 3;
}

void AmfEncoder::encodeInt32(int32_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    WriteUint32BE(m_data.get() + m_index,value);
    m_index += 4;
}

void AmfEncoder::realloc(uint32_t size)
{
    //扩容
    if(size <= m_size)
    {
        return ;
    }

    std::shared_ptr<char> data(new char[size],std::default_delete<char[]>());
    memcpy(data.get(),m_data.get(),m_index);
    m_size = size;
    m_data = data;
}
