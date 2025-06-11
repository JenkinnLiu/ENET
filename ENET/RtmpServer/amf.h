//=============================================================================
// 文件: amf.h
// 描述: 定义 RTMP 使用的 AMF0 (Action Message Format) 编解码接口和数据类型。
//   - 支持基本类型：Number、Boolean、String
//   - 支持复合类型：Object (键值对)、ECMA Array
//   - 用于 RTMP 命令消息（如 connect、play、pause 等）的序列化和解析
//=============================================================================

#ifndef _AMF_H_
#define _AMF_H_

#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <cstdint> // 添加：支持标准整数类型定义

// AMF0DataType: AMF0 格式中每个值前的类型标记 (Marker)
// 对应协议定义，用于指示后续数据的格式和长度
typedef enum
{
    AMF0_NUMBER       = 0x00,  // IEEE-754 双精度浮点
    AMF0_BOOLEAN      = 0x01,  // 1 字节布尔值
    AMF0_STRING       = 0x02,  // 2 字节长度 + UTF-8 字符串
    AMF0_OBJECT       = 0x03,  // 结束标记 0x000009
    AMF0_MOVIECLIP    = 0x04,  /* 保留，未使用 */
    AMF0_NULL         = 0x05,
    AMF0_UNDEFINED    = 0x06,
    AMF0_REFERENCE    = 0x07,
    AMF0_ECMA_ARRAY   = 0x08,  // 4 字节元素个数 + 键值对
    AMF0_OBJECT_END   = 0x09,
    AMF0_STRICT_ARRAY = 0x0A,  // 4 字节长度 + 元素列表
    AMF0_DATE         = 0x0B,  // 8 字节时间戳 + 2 字节时区
    AMF0_LONG_STRING  = 0x0C,  // 4 字节长度 + UTF-8
    AMF0_UNSUPPORTED  = 0x0D,
    AMF0_RECORDSET    = 0x0E,  /* 保留，未使用 */
    AMF0_XML_DOC      = 0x0F,
    AMF0_TYPED_OBJECT = 0x10,
    AMF0_AVMPLUS      = 0x11,  // 切换到 AMF3 编码
    AMF0_INVALID      = 0xFF
} AMF0DataType;

// AmfObjectType: 运行时使用的值类型枚举
// 用于指示解码后保存到 AmfObject 的具体类型
typedef enum
{
    AMF_NUMBER,
    AMF_BOOLEAN,
    AMF_STRING,
} AmfObjectType;

// AmfObject: 内存中的单个 AMF 值对象，用于存储解码后或待编码的基本类型（Number/String/Boolean）
struct AmfObject
{
    AmfObjectType type;      // 运行时类型标识
    std::string   amf_string; // 字符串值
    double        amf_number; // 数字值
    bool          amf_boolean;// 布尔值

    AmfObject() = default;
    AmfObject(const std::string &str)
        : type(AMF_STRING), amf_string(str) {}
    AmfObject(double number)
        : type(AMF_NUMBER), amf_number(number) {}
};

typedef std::unordered_map<std::string, AmfObject> AmfObjects;  // 键值对集合

// AmfDecoder: AMF0 解码器，将二进制 AMF0 数据解析为 AmfObject 或 键值对集合 AmfObjects
class AmfDecoder
{
public:
    /**
     * decode: 解析 AMF0 二进制流，依次读取类型标记并调用对应解码函数
     * @param data  输入字节流起始指针
     * @param size  可用字节数
     * @param n     最多解码的元素个数（-1 表示全部）
     * @return      实际消耗的字节数，小于 size 表示数据不足或已提前结束
     */
    int decode(const char *data, int size, int n = -1);

    // reset: 重置解码器内部状态，清空已解码结果
    void reset();

    // getString: 获取最后一次解码的字符串值
    std::string getString() const;
    // getNumber: 获取最后一次解码的数字值
    double      getNumber() const;
    // hasObject: 检查解码后的对象/数组中是否存在指定键
    bool        hasObject(const std::string &key) const;
    // getObject: 根据键获取对应的 AmfObject 值
    AmfObject   getObject(const std::string &key);
    // getObject: 获取最后一次解码的单值对象
    AmfObject   getObject();
    // getObjects: 获取解码后的键值对集合（对象或 ECMA Array）
    AmfObjects  getObjects();

private:
    // 各类型解码静态方法，返回消耗字节数或错误标志
    static int decodeBoolean(const char *data, int size, bool &amf_boolean);
    static int decodeNumber(const char *data, int size, double &amf_number);
    static int decodeString(const char *data, int size, std::string &amf_string);
    static int decodeObject(const char *data, int size, AmfObjects &amf_objs);
    static uint16_t decodeInt16(const char *data, int size);
    static uint32_t decodeInt24(const char *data, int size);
    static uint32_t decodeInt32(const char *data, int size);

    AmfObject  m_obj;    // 最后一次解码得到的单值对象
    AmfObjects m_objs;   // 解码得到的对象集合或数组
};

// AmfEncoder: AMF0 编码器，将 AmfObject/AmfObjects 序列化为 AMF0 格式字节流
class AmfEncoder
{
public:
    // 构造函数: 初始化编码器，预分配缓冲区
    AmfEncoder(uint32_t size = 1024);
    // 析构函数: 释放资源
    virtual ~AmfEncoder();

    // reset: 重置编码器状态，清除已写入数据
    void reset();
    // data: 获取编码后数据缓冲指针
    std::shared_ptr<char> data();
    // size: 获取当前已写入数据长度
    uint32_t size() const;

    // encodeString: 编码字符串类型
    // @param str      字符串数据指针
    // @param len      字符串字节长度
    // @param isObject 是否写入类型标记（对象属性中设为 false）
    void encodeString(const char* str, int len, bool isObject = true);
    // encodeNumber: 编码数字 (double) 类型
    // @param value 浮点数值
    void encodeNumber(double value);
    // encodeBoolean: 编码布尔类型
    // @param value 0 或 1
    void encodeBoolean(int value);
    // encodeObjects: 编码对象 (键值对集合)
    // @param objs 待编码的键值对集合，支持 Number/String/Boolean
    void encodeObjects(AmfObjects &objs);
    // encodeECMA: 编码 ECMA Array (关联数组)
    // @param objs 待编码的关联数组
    void encodeECMA(AmfObjects &objs);

private:
    // 基础整数写入方法，按大端序存储
    void encodeInt8(int8_t value);
    void encodeInt16(int16_t value);
    void encodeInt24(int32_t value);
    void encodeInt32(int32_t value);
    // realloc: 扩展内部缓冲区大小
    void realloc(uint32_t size);

    std::shared_ptr<char> m_data; // 底层缓冲指针
    uint32_t m_size;              // 缓冲总大小
    uint32_t m_index;             // 当前写入偏移
};

#endif // _AMF_H_