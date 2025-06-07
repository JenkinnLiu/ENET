#include<functional>
#include<string>
#include<memory>

// SocketUtil 类: 提供各种静态方法用于调整套接字的属性和行为
class SocketUtil
{
public:
    // 设置套接字为非阻塞模式
    static void SetNonBlock(int sockfd);
    // 设置套接字为阻塞模式
    static void SetBlock(int sockfd);
    // 允许地址重用，解决服务重启后端口占用问题
    static void SetReuseAddr(int sockfd);
    // 允许端口重用，用于提高系统负载均衡
    static void SetReusePort(int sockfd);
    // 开启套接字保活机制，用于检测空闲连接是否仍然存活
    static void SetKeepAlive(int sockfd);
    // 设置套接字的发送缓冲区大小，优化发送性能
    static void SetSendBufSize(int sockfd, int size);
    // 设置套接字的接收缓冲区大小，优化接收性能
    static void SetRecvBufSize(int sockfd, int size);
};

// TcpSocket 类: 封装 TCP 套接字操作，包括创建、绑定、监听、连接和关闭等
class TcpSocket
{
public:
    // 构造函数：初始化 TcpSocket 对象
    TcpSocket();
    // 虚析构函数：释放资源，确保派生类正确析构
    virtual ~TcpSocket();
    // 创建 TCP 套接字，返回套接字描述符
    int Create();
    // 绑定套接字到指定的 IP 地址和端口
    // 参数 ip: 待绑定的 IP 地址
    // 参数 port: 待绑定的端口号
    bool Bind(std::string ip, short port);
    // 将套接字设置为监听模式
    // 参数 backlog: 连接请求队列容量
    bool Listen(int backlog);
    // 接受一个传入连接，返回新连接的套接字描述符
    int  Accept();
    // 关闭当前套接字
    void Close();
    // 禁止写操作，常用于半关闭连接
    void ShutdownWrite();
    // 获取当前套接字的文件描述符
    int GetSocket() const { return sockfd_; }
private:
    // 套接字描述符，-1 表示未创建或无效
    int sockfd_ = -1;
};