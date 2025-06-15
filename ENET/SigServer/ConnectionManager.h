#include <unordered_map>
#include <memory>
#include <mutex>
#include "../EdoyunNet/TcpConnection.h"
// 信令服务器需要保存每个连接，并通过唯一的标识符区分客户端连接
// 从而通过不同的标识符来获取连接器，通过连接器转发信令信息。
/**
 * @class ConnectionManager
 * @brief 信令服务器的连接管理器，负责添加、移除及查询客户端TCP连接。
 *
 * 使用唯一标识符管理TcpConnection智能指针，保证线程安全访问。
 * 采用单例模式，确保全局只有一个管理器实例。
 */
class ConnectionManager
{
public:
    /**
     * @brief 析构函数，关闭并清理所有连接映射。
     */
    ~ConnectionManager();

    /**
     * @brief 获取单例实例。
     * @return 返回ConnectionManager的唯一实例指针。
     */
    static ConnectionManager* GetInstance();

public:
    /**
     * @brief 添加新的连接。
     * @param idefy 客户端唯一标识符，不能为空。
     * @param conn 要注册的TcpConnection智能指针。
     */
    void AddConn(const std::string& idefy, const TcpConnection::Ptr conn);

    /**
     * @brief 移除已有连接。
     * @param idefy 要移除的连接标识符，不能为空。
     */
    void RmvConn(const std::string& idefy);

    /**
     * @brief 查询指定标识符对应的连接。
     * @param idefy 查询连接的唯一标识符。
     * @return 找到时返回对应的TcpConnection智能指针，否则返回nullptr。
     */
    TcpConnection::Ptr QueryConn(const std::string& idefy);

    /**
     * @brief 获取当前管理的连接数量。
     * @return 返回connMaps_中元素个数。
     */
    uint32_t Size() const { return connMaps_.size(); }

private:
    /**
     * @brief 私有构造函数，禁止外部实例化。
     */
    ConnectionManager();

    /**
     * @brief 关闭并清理所有连接映射。
     */
    void Close();

    std::mutex mutex_; /**< 互斥锁，保护connMaps_在多线程环境下的访问。 */
    //使用unique_ptr实现单例模式
    static std::unique_ptr<ConnectionManager> instance_; /**< 单例实例指针，为nullptr时尚未创建。 */
    //连接管理器 
    std::unordered_map<std::string, TcpConnection::Ptr> connMaps_; /**< 存储id到TcpConnection映射的容器。 */
};