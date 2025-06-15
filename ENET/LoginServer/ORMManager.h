#ifndef _ORMMANAGER_H_
#define _ORMMANAGER_H_
#include <memory>
#include <mutex>
#include <mysql/mysql.h>

/**
 * @file ORMManager.h
 * @brief ORMManager 单例类，封装对 MySQL clients 表的增删查操作。
 *
 * 内部维护 MySQL 连接，提供用户注册、登录查询、注销等接口，
 * 并确保通过线程安全的单例模式唯一实例化。
 */

/**
 * @class ORMManager
 * @brief 管理用户信息的 ORM 操作类
 */
class ORMManager
{
public:
    /**
     * @brief 析构函数，关闭并清理 MySQL 连接资源
     */
    ~ORMManager();

    /**
     * @brief 获取 ORMManager 单例实例
     * @return 唯一实例指针
     */
    static ORMManager* GetInstance();

    /**
     * @brief 用户注册接口，将新用户信息插入数据库
     * @param name 用户姓名
     * @param acount 用户账号
     * @param password 用户密码
     * @param usercode 用户唯一标识
     * @param sig_server 信令服务器地址
     */
    void UserRegister(const char* name, const char* acount, const char* password,
                      const char* usercode, const char* sig_server);

    /**
     * @brief 用户登录查询，根据 usercode 返回对应数据库行
     * @param usercode 用户唯一标识
     * @return MYSQL_ROW 查询结果行，失败时返回 NULL
     */
    MYSQL_ROW UserLogin(const char* usercode);

    /**
     * @brief 注销用户接口，根据 usercode 删除对应记录
     * @param usercode 用户唯一标识
     */
    void UserDestroy(const char* usercode);

    /**
     * @brief 内部方法，插入新的 client 记录到数据库
     * @param name 用户姓名
     * @param acount 用户账号
     * @param password 用户密码
     * @param usercode 用户唯一标识
     * @param online 在线状态标记
     * @param recently_login 最近登录时间戳
     * @param sig_server 信令服务器地址
     */
    void insertClient(const char* name, const char* acount, const char* password,
                      const char* usercode, int online, long recently_login,
                      const char* sig_server);

protected:
    /**
     * @brief 内部方法，根据 usercode 删除 client 记录
     * @param usercode 用户唯一标识
     */
    void deleteClientByUsercode(const char* usercode);

    /**
     * @brief 内部方法，根据 usercode 查询 client 记录
     * @param usercode 用户唯一标识
     * @return MYSQL_ROW 查询结果行，失败时返回 NULL
     */
    MYSQL_ROW selectClientByUsercode(const char* usercode);

private:
    /**
     * @brief 私有构造函数，初始化并连接到 MySQL 数据库
     */
    ORMManager();

    MYSQL mysql_;  ///< MySQL 连接句柄

private:
    static std::unique_ptr<ORMManager> instance_;  ///< 单例实例指针
};
#endif