#ifndef H264PARASER_H
#define H264PARASER_H
#include <cstdint>
#include <utility>

class H264Paraser
{
public:
    typedef std::pair<uint8_t*,uint8_t*> Nal;//这两个指针分别指向这个Nal的头和尾
    H264Paraser();
    static Nal findNal(const uint8_t* data,uint32_t size);
};

#endif // H264PARASER_H
