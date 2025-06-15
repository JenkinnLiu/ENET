#include "define.h"

/**
 * @file TcpClient.h
 * @brief 客户端监控组件，负责采集本机内存使用率并通过 TCP 发送给负载均衡服务器
 *
 * 功能：
 *  1. 打开 /proc/meminfo 文件并读取系统内存信息
 *  2. 计算内存使用率（percentage）
 *  3. 通过 TCP 与负载均衡服务器建立连接并发送心跳包
 */
class TcpClient
{
public:
    /**
     * @brief 构造函数，初始化成员变量（socket 描述符、文件指针、连接状态）
     */
    TcpClient();

    /**
     * @brief 初始化客户端，打开 /proc/meminfo 文件并创建 TCP socket，
     *        并设置默认监控目标地址
     */
    void Create();

    /**
     * @brief 与指定服务器建立 TCP 连接
     * @param ip   服务器 IP 地址（点分十进制字符串）
     * @param port 服务器监听端口
     * @return 成功返回 true，失败返回 false
     */
    bool Connect(std::string ip, uint16_t port);

    /**
     * @brief 关闭客户端连接，关闭文件及 socket
     */
    void Close();

    /**
     * @brief 析构函数，自动关闭资源
     */
    ~TcpClient();

    /**
     * @brief 对外触发监控信息发送接口，内部调用 get_mem_usage()
     */
    void getMonitorInfo();  

protected:
    /**
     * @brief 读取并计算内存使用率，同时发送心跳包到服务器
     */
    void get_mem_usage();

    /**
     * @brief 通过 TCP  socket 发送数据
     * @param data 要发送的数据缓冲
     * @param size 缓冲区长度（字节数）
     */
    void Send(uint8_t* data, uint32_t size);

private:
    FILE* file_;               ///< 打开的 /proc/meminfo 文件指针
    bool isConnect_;           ///< TCP 连接状态标志
    int sockfd_;               ///< TCP socket 描述符
    struct sysinfo info_;      ///< 系统内存总量信息，用于计算使用率
    Monitor_body Monitor_info_;///< 心跳包中包含的监控数据结构体
};