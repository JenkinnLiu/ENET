//=============================================================================
// 文件: amf.cpp
// 描述: 实现 RTMP 协议中 AMF0(Action Message Format) 的编解码。
//   - AmfDecoder: 将网络字节流中的 AMF0 数据解析为内存对象
//   - AmfEncoder: 将内存对象序列化为 AMF0 格式字节流
// 主要流程:
//   解码: 根据类型标记(AMF0DataType)依次调用 decodeBoolean/Number/String/Object
//   编码: 按照 AMF0 规范写入类型标记 + 数据长度/值
//=============================================================================

#include "amf.h"
#include "../EdoyunNet/BufferReader.h"
#include "../EdoyunNet/BufferWriter.h"
#include <string.h>

// decode: 主解码入口，遍历字节流读取类型标记并调用对应方法解析，累计消耗字节数
// @param data: 输入 AMF0 二进制流缓冲
// @param size: 可用字节数
// @param n: 最大解码条目数(-1 表示全部)
// @return 返回已消耗字节数，<size 表示有剩余或数据不足
int AmfDecoder::decode(const char *data, int size, int n)
{
    int bytes_used = 0;
    while (bytes_used < size)
    {
        int ret = 0;
        char type = data[bytes_used]; // 读取类型标记
        bytes_used += 1;

        switch (type)
        {
        case AMF0_NUMBER:  // 数字类型
            m_obj.type = AMF_NUMBER;
            ret = decodeNumber(data + bytes_used,size - bytes_used,m_obj.amf_number);
            break;
        case AMF0_BOOLEAN: // 布尔类型
            m_obj.type = AMF_BOOLEAN;
            ret = decodeBoolean(data + bytes_used,size - bytes_used,m_obj.amf_boolean);
            break;
        case AMF0_STRING:  // 短字符串类型
            m_obj.type = AMF_STRING;
            ret = decodeString(data + bytes_used,size - bytes_used,m_obj.amf_string);
            break;
        case AMF0_OBJECT:  // 对象类型 (键-值对)
            ret = decodeObject(data + bytes_used,size - bytes_used,m_objs);
            break;
        case AMF0_ECMA_ARRAY: // ECMA Array (关联数组)
            // 跳过 4 字节元素计数，再解析对象内容
            ret = decodeObject(data + bytes_used + 4,size - bytes_used - 4,m_objs);
        default:
            break;
        }

        if(ret < 0) // 数据不足或解析错误，退出
        {
            break;
        }

        bytes_used += ret;
        n--;
        if(n == 0) // 达到解码条目上限
        {
            break;
        }
    }

    return bytes_used;
}

// decodeBoolean: 解析布尔类型，1 字节，非零即真
// @param data 输入起始地址
// @param size 缓冲长度，应至少为1
// @param amf_boolean 输出布尔值
// @return 消耗字节数(1)或0(数据不足)
int AmfDecoder::decodeBoolean(const char *data, int size, bool &amf_boolean)
{
    if(size < 1) // 数据不足
    {
        return 0;
    }

    amf_boolean = (data[0] != 0);
    return 1; // 消耗 1 字节
}

// decodeNumber: 解析 8 字节 IEEE-754 双精度浮点，按大端存储
// @param data 输入地址
// @param size 缓冲长度，应>=8
// @param amf_number 输出数值
// @return 消耗字节数(8)或0(数据不足)
int AmfDecoder::decodeNumber(const char *data, int size, double &amf_number)
{
    if(size < 8) // 数据不足
    {
        return 0;
    }

    // 大端转小端，逐字节拷贝
    char* ci = (char*)data;
    char* co = (char*)&amf_number;
    for(int i = 0; i < 8; ++i)
    {
        co[i] = ci[7 - i];
    }

    return 8; // 消耗 8 字节
}

// decodeString: 解析短字符串 (2 字节长度 + UTF-8 数据)
// @return 正数: 消耗总字节数; 0: 长度字段不足; -1: 数据不完整
int AmfDecoder::decodeString(const char *data, int size, std::string &amf_string)
{
    if(size < 2) // 长度字段不完整
    {
        return 0;
    }

    int bytes_used = 0;
    int strSize = decodeInt16(data,size); // 读取长度
    bytes_used += 2;
    if(strSize > (size - bytes_used)) // 数据不足
    {
        return -1;
    }

    amf_string.assign(data + bytes_used, strSize);
    bytes_used += strSize;
    return bytes_used;
}

// decodeObject: 解析对象或 ECMA Array，读取键名 + 值，存入 amf_objs
// 从 data 中不断读取键名(key)+单个 Value, 存入 amf_objs
// @return 消耗字节数，>0 表示有数据，<=0 表示错误或结束
int AmfDecoder::decodeObject(const char *data, int size, AmfObjects &amf_objs)
{
    amf_objs.clear();
    int bytes_used = 0;
    while(size > 0)
    {
        int strLen = decodeInt16(data + bytes_used,size); // 键名长度
        size -= 2;
        if(size < strLen) // 数据不足，结束
        {
            return bytes_used;
        }

        std::string key(data + bytes_used + 2, strLen); // 读取键名
        size -= strLen;

        // 解析值字段
        AmfDecoder dec;
        int ret = dec.decode(data + bytes_used + 2 + strLen,size,1);
        bytes_used += 2 + strLen + ret;
        if(ret <= 1) // 值解析异常或结束
        {
            break;
        }
        amf_objs.emplace(key, dec.getObject());
    }
    return bytes_used;
}

// decodeInt16/24/32: 按大端读取定长整数
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

// 构造函数: 预分配编码缓冲区
AmfEncoder::AmfEncoder(uint32_t size)
    : m_data(new char[size], std::default_delete<char[]>()), m_size(size), m_index(0)
{
}

AmfEncoder::~AmfEncoder()
{
}

// encodeString: 序列化字符串类型
// @param str      字符串数据指针
// @param len      字符串字节长度
// @param isObject 是否写入类型标记（对象属性中设为 false）
// 逻辑:
//   - 检查剩余缓冲是否足够，否则扩容
// - 根据 len 决定使用短字符串(AMF0_STRING)或长字符串(AMF0_LONG_STRING)
//   - 写入类型标记和长度字段
//   - 复制字符串内容
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

// encodeNumber: 序列化数字 (1 字节类型标记 + 8 字节 IEEE-754 double)
// @param value 双精度浮点数值
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

// encodeBoolean: 序列化布尔类型 (1 字节类型标记 + 1 字节值)
// @param value 0 表示 false，非 0 表示 true
void AmfEncoder::encodeBoolean(int value)
{
    if((m_size - m_index) < 2) //1字节类型 bool 1字节
    {
        this->realloc(m_size + 1024);
    }

    m_data.get()[m_index++] = AMF0_BOOLEAN;
    m_data.get()[m_index++] = value ? 0x01 : 0x00;
}

// encodeObjects: 序列化对象 (键值对)
// @param objs 待编码的键值对集合
// 逻辑:
//   - 对象为空时写入 AMF0_NULL 并返回
//   - 写入对象起始标记 AMF0_OBJECT
//   - 遍历每个键值:
//       - encodeString(key, key.size(), false)
//       - 根据类型调用 encodeNumber/encodeString/encodeBoolean
//   - 写入空键和 AMF0_OBJECT_END 结束标记
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

// encodeECMA: 序列化 ECMA Array (关联数组)
// @param objs 待编码的关联数组
// 逻辑:
//   - 写入类型标记 AMF0_ECMA_ARRAY
//   - 写入 4 字节元素计数（此处写 0 可在解码时根据结束标记判断）
//   - 遍历 objs 编码键值，流程同 encodeObjects
//   - 写入空键和 AMF0_OBJECT_END 结束标记
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

// encodeInt8: 写入 1 字节大端整数
// @param value 8 位整数
void AmfEncoder::encodeInt8(int8_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    m_data.get()[m_index++] = value;
}

// encodeInt16: 写入 2 字节大端整数
// @param value 16 位整数
void AmfEncoder::encodeInt16(int16_t value)
{
    if((m_size - m_index) < 2)
    {
        this->realloc(m_size + 1024);
    }

    WriteUint16BE(m_data.get() + m_index,value);
    m_index += 2;
}

// encodeInt24: 写入 3 字节大端整数
// @param value 24 位整数
void AmfEncoder::encodeInt24(int32_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    
    WriteUint24BE(m_data.get() + m_index,value);
    m_index += 3;
}

// encodeInt32: 写入 4 字节大端整数
// @param value 32 位整数
void AmfEncoder::encodeInt32(int32_t value)
{
    if((m_size - m_index) < 1)
    {
        this->realloc(m_size + 1024);
    }
    WriteUint32BE(m_data.get() + m_index,value);
    m_index += 4;
}

// realloc: 扩展内部缓冲区大小
// @param size 新缓冲区大小
// 逻辑: 如果新大小 <= 当前大小，则不做处理；否则分配新内存并拷贝已有数据
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
