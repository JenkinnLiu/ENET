#include "TcpClient.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * @brief 构造函数，初始化 socket 描述符、文件指针和连接状态
 */
TcpClient::TcpClient()
    : sockfd_(-1)
    , file_(nullptr)
    , isConnect_(false)
{
}

/**
 * @brief 初始化客户端资源：
 * - 打开 /proc/meminfo 以获取内存信息
 * - 调用 sysinfo 初始化总内存信息
 * - 创建 TCP socket
 * - 设置目标服务器地址和端口到 Monitor_info_
 */
void TcpClient::Create()
{
    file_ = fopen("/proc/meminfo", "r");
    if (!file_)
    {
        printf("open file failed\n");
        return;
    }
    memset(&info_, 0, sizeof(info_));
    sysinfo(&info_);

    sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    Monitor_info_.SetIp("192.168.31.30");
    Monitor_info_.port = 9867;
}

/**
 * @brief 建立与服务器的 TCP 连接
 * @param ip   服务器 IP
 * @param port 服务器端口
 * @return 连接成功返回 true，失败返回 false
 */
bool TcpClient::Connect(std::string ip, uint16_t port)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(sockfd_, (sockaddr*)&addr, sizeof(addr)) == -1)
    {
        printf("connect error\n");
        return false;
    }
    isConnect_ = true;
    return true;
}

/**
 * @brief 关闭客户端资源：
 * - 关闭文件
 * - 关闭 socket
 * - 重置连接状态
 */
void TcpClient::Close()
{
    if (isConnect_)
    {
        fclose(file_);
        ::close(sockfd_);
        isConnect_ = false;
    }
}

/**
 * @brief 析构函数，自动调用 Close() 释放资源
 */
TcpClient::~TcpClient()
{
    Close();
}

/**
 * @brief 获取并发送一次监控心跳
 * 对外公开接口，内部调用 get_mem_usage()
 */
void TcpClient::getMonitorInfo()
{
    get_mem_usage();
}

/**
 * @brief 读取 /proc/meminfo 计算内存使用率并构造并发送心跳包
 * - 跳过前两行
 * - 查找 "MemAvailable" 字段
 * - 计算 (total - available) / total * 100
 * - 填充 Monitor_info_.mem 并调用 Send()
 */
void TcpClient::get_mem_usage()
{
    size_t bytes_used;
    char* line = nullptr;
    int index = 0;
    int avimem = 0;

    while (getline(&line, &bytes_used, file_) != -1)
    {
        if (++index <= 2)
            continue;
        if (strstr(line, "MemAvailable") != nullptr)
        {
            sscanf(line, "%*s %d %*s", &avimem);
            break;
        }
    }
    int total_kb = info_.totalram / 1024;
    double mem_pct = (total_kb - avimem) * 100.0 / total_kb;
    Monitor_info_.mem = static_cast<uint8_t>(mem_pct);
    Send(reinterpret_cast<uint8_t*>(&Monitor_info_), Monitor_info_.len);
}

/**
 * @brief 通过 TCP socket 循环发送完整缓冲
 * @param data 缓冲起始地址
 * @param size 缓冲长度
 */
void TcpClient::Send(uint8_t *data, uint32_t size)
{
    int sent = 0;
    while (sent < size)
    {
        int n = ::send(sockfd_, data + sent, size - sent, 0);
        if (n <= 0)
            break;
        sent += n;
    }
}
