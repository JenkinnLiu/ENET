#include "ORMManager.h"
#include <chrono>

std::unique_ptr<ORMManager> ORMManager::instance_ = nullptr;

/**
 * @brief 构造函数，初始化 MySQL 客户端并连接数据库
 * - 使用 mysql_init 初始化句柄
 * - 调用 mysql_real_connect 连接到指定服务器、数据库
 * - 连接失败时打印错误信息
 */
ORMManager::ORMManager()
{
    mysql_init(&mysql_);
    if (mysql_real_connect(&mysql_, "192.168.31.30", "root", "123456", "users", 3306, NULL, 0) == NULL)
    {
        printf("连接 MySQL 服务器失败: %s\n", mysql_error(&mysql_));
        return;
    }
    printf("连接成功\n");
}

/**
 * @brief 析构函数，关闭 MySQL 连接并释放资源
 */
ORMManager::~ORMManager()
{
    mysql_close(&mysql_);
}

/**
 * @brief 获取并初始化单例实例
 * - 使用 std::call_once 确保线程安全的单例创建
 * @return 唯一 ORMManager 实例指针
 */
ORMManager* ORMManager::GetInstance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        instance_.reset(new ORMManager());
    });
    return instance_.get();
}

/**
 * @brief 用户注册接口，实现新用户插入逻辑
 * - 获取当前系统时间戳
 * - 调用 insertClient 增加记录
 */
void ORMManager::UserRegister(const char *name, const char *acount, const char *password,
                               const char *usercode, const char *sig_server)
{
    auto now = std::chrono::system_clock::now();
    auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    insertClient(name, acount, password, usercode, 0, nowTimestamp, sig_server);
}

/**
 * @brief 用户登录接口，查询指定 usercode 的用户记录
 * @param usercode 用户唯一标识
 * @return MYSQL_ROW 如果找到返回结果行，否则返回 NULL
 */
MYSQL_ROW ORMManager::UserLogin(const char *usercode)
{
    return selectClientByUsercode(usercode);
}

/**
 * @brief 用户注销接口，根据 usercode 删除对应数据库记录
 */
void ORMManager::UserDestroy(const char *usercode)
{
    deleteClientByUsercode(usercode);
}

/**
 * @brief 私有方法，向 clients 表插入新用户记录
 * @param name 用户姓名
 * @param acount 用户账号
 * @param password 用户密码
 * @param usercode 用户唯一标识
 * @param online 在线状态 (0 离线、1 在线)
 * @param recently_login 最近登录时间戳
 * @param sig_server 信令服务器地址
 */
void ORMManager::insertClient(const char *name, const char *acount, const char *password,
                              const char *usercode, int online, long recently_login,
                              const char *sig_server)
{
    char query[1024];
    sprintf(query,
            "INSERT INTO clients (USER_NAME, USER_ACOUNT, USER_PASSWD, USER_CODE, USER_ONLINE, USER_RECENTLY_LOGIN, USER_SVR_MOUNT) "
            "VALUES ('%s', '%s', '%s', '%s', '%d', '%ld', '%s')",
            name, acount, password, usercode, online, recently_login, sig_server);
    if (mysql_query(&mysql_, query))
    {
        printf("Insert failed: %s\n", mysql_error(&mysql_));
    }
    else
    {
        printf("Insert successful\n");
    }
}

/**
 * @brief 私有方法，根据 usercode 删除 clients 表中对应记录
 * @param usercode 用户唯一标识
 */
void ORMManager::deleteClientByUsercode(const char *usercode)
{
    char query[1024];
    sprintf(query, "DELETE FROM clients WHERE USER_CODE = '%s'", usercode);
    if (mysql_query(&mysql_, query))
    {
        printf("Delete failed: %s\n", mysql_error(&mysql_));
    }
    else
    {
        printf("Delete successful\n");
    }
}

/**
 * @brief 私有方法，根据 usercode 查询 clients 表并返回首行结果
 * @param usercode 用户唯一标识
 * @return MYSQL_ROW 查询到的首行记录，查询失败或无记录时返回 NULL
 */
MYSQL_ROW ORMManager::selectClientByUsercode(const char *usercode)
{
    char query[1024];
    sprintf(query, "SELECT * FROM clients WHERE USER_CODE = '%s'", usercode);
    if (mysql_query(&mysql_, query))
    {
        printf("Select failed: %s\n", mysql_error(&mysql_));
        return NULL;
    }

    MYSQL_RES* res = mysql_store_result(&mysql_);
    if (res)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        mysql_free_result(res);
        return row;
    }

    return NULL;
}
