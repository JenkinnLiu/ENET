#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <chrono>
class Timestamp
{
public:
    Timestamp()
        : begin_time_point_(std::chrono::high_resolution_clock::now())
    { }

    void Reset()
    {
        begin_time_point_ = std::chrono::high_resolution_clock::now();
    }

    int64_t Elapsed()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin_time_point_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_time_point_;
};
#endif // TIMESTAMP_H
