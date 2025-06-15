/**
 * @file ConnectionManager.cpp
 * @brief ConnectionManager类实现文件，包含单例实例及连接管理方法的定义。
 */

#include "ConnectionManager.h"

/**
 * @brief 初始化单例实例指针unique_ptr，默认为空指针
 */
std::unique_ptr<ConnectionManager> ConnectionManager::instance_ = nullptr;

/**
 * @brief 私有构造函数，禁止外部直接实例化
 */
ConnectionManager::ConnectionManager()
{
}

/**
 * @brief 析构函数，负责调用Close清理所有连接资源
 */
ConnectionManager::~ConnectionManager()
{
    Close();
}

/**
 * @brief 获取ConnectionManager单例实例
 *        使用std::call_once确保线程安全的初始化
 * @return 单例实例指针
 */
ConnectionManager* ConnectionManager::GetInstance()
{
    static std::once_flag flag;
    std::call_once(flag,[&](){
        instance_.reset(new ConnectionManager()); //只会创建一次
    });
    return instance_.get();
}

/**
 * @brief 添加新的TCP连接到管理器
 * @param idefy 客户端连接唯一标识符，不能为空
 * @param conn  要管理的TcpConnection智能指针
 */
void ConnectionManager::AddConn(const std::string &idefy, const TcpConnection::Ptr conn)
{
    if(idefy.empty())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it == connMaps_.end()) //说明这个连接器未加入
    {
        connMaps_.emplace(idefy,conn);
    }
}

/**
 * @brief 从管理器中移除指定连接
 * @param idefy 要移除的连接标识符，不能为空
 */
void ConnectionManager::RmvConn(const std::string &idefy)
{
    if(idefy.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it != connMaps_.end())//查询到
    {
        connMaps_.erase(idefy);
    }
}

/**
 * @brief 查询指定标识符对应的TcpConnection连接
 * @param idefy 要查询的连接标识符
 * @return 找到时返回对应的TcpConnection智能指针，否则返回nullptr
 */
TcpConnection::Ptr ConnectionManager::QueryConn(const std::string &idefy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connMaps_.find(idefy);
    if(it != connMaps_.end())
    {
        return it->second;
    }
    return nullptr;
}

/**
 * @brief 清理所有连接映射，释放所有资源
 */
void ConnectionManager::Close()
{
    connMaps_.clear();
}
