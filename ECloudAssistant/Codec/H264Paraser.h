/**
 * @file H264Paraser.h
 * @brief H264比特流NAL单元解析类定义
 *
 * 定义了H264Paraser类用于查找和提取H264视频流中的NAL单元，
 * 支持三字节（00 00 01）和四字节（00 00 00 01）起始码模式。
 */

#ifndef H264PARASER_H
#define H264PARASER_H

#include <cstdint>
#include <utility>

/**
 * @class H264Paraser
 * @brief H264比特流NAL单元解析工具类
 *
 * 提供静态方法findNal，用于在原始字节流中查找第一个NAL单元的
 * 起始和结束位置，可用于提取SPS/PPS及视频帧数据。
 */
class H264Paraser
{
public:
    /**
     * @typedef Nal
     * @brief NAL单元边界指针类型
     *
     * pair.first 为NAL单元数据起始地址（跳过起始码）；
     * pair.second 为NAL单元数据结束地址（不包含下一个起始码）。
     */
    typedef std::pair<uint8_t*,uint8_t*> Nal; // 这两个指针分别指向NAL单元的头和尾

    /**
     * @brief 默认构造函数
     */
    H264Paraser();

    /**
     * @brief 查找首个NAL单元
     * @param data 输入H264字节流缓冲区指针
     * @param size 缓冲区长度（字节数）
     * @return 返回Nal(first, second)，分别标记NAL单元数据区间；
     *         若未找到则返回(Nal(nullptr, nullptr))。
     *
     * 函数内部持续检测三字节("00 00 01")和四字节("00 00 00 01")
     * 起始码，自动跳过起始码并标记NAL单元的起始与结束位置。
     */
    static Nal findNal(const uint8_t* data, uint32_t size);
};

#endif // H264PARASER_H
