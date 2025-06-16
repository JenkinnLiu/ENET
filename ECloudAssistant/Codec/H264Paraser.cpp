#include "H264Paraser.h"
#include <cstring>

H264Paraser::H264Paraser()
{

}

H264Paraser::Nal H264Paraser::findNal(const uint8_t *data, uint32_t size)
{
    Nal nal(nullptr,nullptr);
    if(size < 5) //因为sps或者pps会大于5，因为这个startcode就是3-4字节，而且这个pps四字节
    {
        return nal;
    }

    nal.second = const_cast<uint8_t*>(data) + (size -1);

    uint32_t startCode = 0;
    uint32_t pos = 0;
    uint8_t prefix[3] = {0};
    memcpy(prefix,data,3);

    size -= 3;
    data += 2;

    while(size --)
    {
        if((prefix[pos % 3]) == 0 && (prefix[(pos + 1)% 3]) == 0 && (prefix[(pos + 2)% 3]) == 1)
        {
            //00 00 01
            if(nal.first == nullptr)
            {
                nal.first = const_cast<uint8_t*>(data) + 1;//偏移这个startcode
                startCode = 3;
            }
            //退出
            else if(startCode == 3) //就说明找到了下一个startcode
            {
                //更新尾部
                nal.second = const_cast<uint8_t*>(data) -3;//减去前缀
            }
        }
        else if((prefix[pos % 3]) == 0 && (prefix[(pos + 1)% 3]) == 0 && (prefix[(pos + 2)% 3]) == 0)
        {
            //00 00 00 01
            if(*(data + 1) == 0x01)
            {
                if(nal.first == nullptr)
                {
                    if(size >= 1)
                    {
                        nal.first = const_cast<uint8_t*>(data) + 2;//偏移四字节，
                    }
                    else
                    {
                        break;
                    }
                    startCode = 4;
                }
                else if(startCode == 4)//找到下一个nal头
                {
                    //更新尾部
                    nal.second = const_cast<uint8_t*>(data) - 3;//前缀
                    break;
                }
            }
        }
        //更新前缀
        prefix[(pos++) % 3] = *(++data);
    }
    if(nal.first == nullptr)
    {
        nal.second == nullptr;
    }
    return nal;
}
