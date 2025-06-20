/**
 * @file AudioBuffer.h
 * @brief 环形缓冲区实现，用于存储和读取 PCM 音频数据，支持线程安全写入和读取
 * @date 2025-06-18
 */
#ifndef AUIDO_BUFFER_H
#define AUIDO_BUFFER_H
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

/**
 * @class AudioBuffer
 * @brief 提供可变长度的缓冲区，支持写入、读取和清空操作，内部自动管理读写索引和数据移动
 */
class AudioBuffer
{
public:
    /**
     * @brief 构造函数，初始化缓冲区大小并分配存储空间
     * @param size 缓冲区初始容量（字节）
     */
    AudioBuffer(uint32_t size = 1024)
        : _bufferSize(size)
    {
        _buffer.resize(size);
    }

    /**
     * @brief 析构函数，负责资源清理
     */
    ~AudioBuffer()
    {

    }

    /**
     * @brief 将外部数据写入缓冲区
     * @param data 待写入数据指针
     * @param size 写入数据大小（字节）
     * @return 实际写入的字节数
     */
    int write(const char *data, uint32_t size)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t bytes = writableBytes();

        if (bytes < size)
        {
            size = bytes;
        }

        if (size > 0)
        {
            memcpy(beginWrite(), data, size);// 将数据复制到缓冲区写入位置
            // 更新写索引
            _writerIndex += size;
        }

        retrieve(0);
        return size;
    }

    /**
     * @brief 从缓冲区读取数据
     * @param data 目标缓冲区指针
     * @param size 要读取的字节数
     * @return 成功读取的字节数，失败返回 -1
     */
    int read(char *data, uint32_t size)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (size > readableBytes())
        {
            retrieve(0);
            return -1;
        }

        memcpy(data, peek(), size);
        retrieve(size);
        return size;
    }

    /**
     * @brief 获取当前可读取的数据量
     * @return 可读取字节数
     */
    uint32_t size()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return readableBytes();
    }

    /**
     * @brief 清空缓冲区，重置读写索引
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        retrieveAll();
    }

private:
    /**
     * @brief 获取当前可读字节数
     * @return 可读字节数
     */
    uint32_t readableBytes() const
    {
        return _writerIndex - _readerIndex;
    }

    /**
     * @brief 获取当前可写字节数
     * @return 可写字节数
     */
    uint32_t writableBytes() const
    {
        return _buffer.size() - _writerIndex;
    }

    /**
     * @brief 获取当前读指针位置
     * @return 指向可读数据的指针
     */
    char* peek()
    {
        return begin() + _readerIndex;
    }

    /**
     * @brief 获取常量读指针位置
     * @return 指向可读数据的常量指针
     */
    const char* peek() const
    {
        return begin() + _readerIndex;
    }

    /**
     * @brief 重置读写索引，清空所有数据
     */
    void retrieveAll()
    {
        _writerIndex = 0;
        _readerIndex = 0;
    }

    /**
     * @brief 移动读索引，删除指定长度数据
     * @param len 要丢弃的字节数
     */
    void retrieve(size_t len)
    {
        if (len > 0)
        {
            if (len <= readableBytes())
            {
                _readerIndex += len;// 更新读索引, 向前移动
                if (_readerIndex == _writerIndex)
                {
                    _readerIndex = 0;
                    _writerIndex = 0;
                }
            }
        }

        if (_readerIndex > 0 && _writerIndex > 0)
        {
            _buffer.erase(_buffer.begin(), _buffer.begin() + _readerIndex);
            _buffer.resize(_bufferSize);
            _writerIndex -= _readerIndex;
            _readerIndex = 0;
        }
    }

    /**
     * @brief 读取直到指定位置，并更新读指针
     * @param end 指向目标位置的指针
     */
    void retrieveUntil(const char* end)
    {
        retrieve(end - peek());
    }

    /**
     * @brief 获取缓冲区底层数组起始指针
     * @return 指向缓冲区数据起始位置的指针
     */
    char* begin()
    {
        return &*_buffer.begin();
    }

    /**
     * @brief 获取常量缓冲区底层数组起始指针
     * @return 指向缓冲区数据起始位置的常量指针
     */
    const char* begin() const
    {
        return &*_buffer.begin();
    }

    /**
     * @brief 计算写入位置指针
     * @return 指向写入位置的指针
     */
    char* beginWrite()
    {
        return begin() + _writerIndex;
    }

    /**
     * @brief 常量写入位置指针
     * @return 指向写入位置的常量指针
     */
    const char* beginWrite() const
    {
        return begin() + _writerIndex;
    }

    std::mutex _mutex;            ///< 线程安全保护锁
    uint32_t _bufferSize = 0;     ///< 缓冲区容量
    std::vector<char> _buffer;    ///< 数据存储容器
    size_t _readerIndex = 0;      ///< 读索引
    size_t _writerIndex = 0;      ///< 写索引
};
#endif
