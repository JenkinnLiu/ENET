/**
 * @file define.h
 * @brief 登录服务器与监控模块的协议定义文件。
 *
 * 包含客户端与服务器通信所用的命令（Cmd）、返回码（ResultCode）、
 * 以及所有数据包头和具体包体结构体定义。使用 #pragma pack(1) 确保网络传输对齐。
 */

#ifndef _DEFINE_H_
#define _DEFINE_H_
#include <cstdint>
#include <string>
#include <array>
#include <sys/sysinfo.h>

#pragma pack(push,1)

/**
 * @enum Cmd
 * @brief 信令协议命令类型枚举
 */
enum Cmd : uint16_t
{
    Minotor,   ///< 心跳/监控信息上报命令
    ERROR,     ///< 通用错误命令
    Login,     ///< 用户登录请求命令
    Register,  ///< 用户注册请求命令
    Destory,   ///< 用户注销请求命令
};

/**
 * @enum ResultCode
 * @brief 服务器对请求的处理结果状态码
 */
enum ResultCode
{
    S_OK = 0,            ///< 操作成功
    SERVER_ERROR,        ///< 服务器内部错误
    REQUEST_TIMEOUT,     ///< 请求超时
    ALREADY_REDISTERED,  ///< 已注册
    USER_DISAPPEAR,      ///< 用户不存在
    ALREADY_LOGIN,       ///< 已登录
    VERFICATE_FAILED     ///< 验证失败
};

/**
 * @struct packet_head
 * @brief 所有协议包的公共包头，包含包长和命令类型
 */
struct packet_head {
    packet_head()
        : len(0), cmd(0) {}
    uint16_t len;  ///< 包体总长度（含头）
    uint16_t cmd;  ///< 命令类型，对应 Cmd 枚举
};

/**
 * @struct UserRegister
 * @brief 用户注册请求包体
 */
struct UserRegister : public packet_head
{
    UserRegister() : packet_head()
    {
        cmd = Register;
        len = sizeof(UserRegister);
    }
    void SetCode(const std::string& str) { str.copy(code.data(), code.size()); }
    std::string GetCode() const { return std::string(code.data()); }
    void SetName(const std::string& str) { str.copy(name.data(), name.size()); }
    std::string GetName() const { return std::string(name.data()); }
    void SetCount(const std::string& str) { str.copy(count.data(), count.size()); }
    std::string GetCount() const { return std::string(count.data()); }
    void SetPasswd(const std::string& str) { str.copy(passwd.data(), passwd.size()); }
    std::string GetPasswd() const { return std::string(passwd.data()); }

    std::array<char,20> code;    ///< 用户唯一标识
    std::array<char,20> name;    ///< 用户姓名
    std::array<char,12> count;   ///< 账号
    std::array<char,20> passwd;  ///< 密码（明文或初级加密）
    uint64_t timestamp;          ///< 请求发送时的时间戳（毫秒）
};

/**
 * @struct UserLogin
 * @brief 用户登录请求包体
 */
struct UserLogin : public packet_head
{
    UserLogin() : packet_head()
    {
        cmd = Login;
        len = sizeof(UserLogin);
    }
    void SetCode(const std::string& str) { str.copy(code.data(), code.size()); }
    std::string GetCode() const { return std::string(code.data()); }
    void SetCount(const std::string& str) { str.copy(count.data(), count.size()); }
    std::string GetCount() const { return std::string(count.data()); }
    void SetPasswd(const std::string& str) { str.copy(passwd.data(), passwd.size()); }
    std::string GetPasswd() const { return std::string(passwd.data()); }

    std::array<char,20> code;    ///< 用户唯一标识
    std::array<char,12> count;   ///< 登录账号
    std::array<char,33> passwd;  ///< 密码（MD5 或其它散列值）
    uint64_t timestamp;          ///< 请求发送时的时间戳
};

/**
 * @struct RegisterResult
 * @brief 用户注册应答包体
 */
struct RegisterResult : public packet_head
{
    RegisterResult() : packet_head()
    {
        cmd = Register;
        len = sizeof(RegisterResult);
    }
    ResultCode resultCode; ///< 注册操作结果码
};

/**
 * @struct LoginResult
 * @brief 用户登录应答包体，包含结果码和信令服务器地址
 */
struct LoginResult : public packet_head
{
    LoginResult() : packet_head()
    {
        cmd = Login;
        len = sizeof(LoginResult);
    }
    void SetIp(const std::string& str) { str.copy(ctrSvrIp.data(), ctrSvrIp.size()); }
    std::string GetIp() const { return std::string(ctrSvrIp.data()); }

    ResultCode resultCode;      ///< 登录操作结果码
    uint16_t port;              ///< 分配的信令服务器端口
    std::array<char,16> ctrSvrIp;///< 分配的信令服务器 IP
};

/**
 * @struct UserDestory
 * @brief 用户注销请求包体
 */
struct UserDestory : public packet_head
{
    UserDestory() : packet_head()
    {
        cmd = Destory;
        len = sizeof(UserDestory);
    }
    void SeCode(const std::string& str) { str.copy(code.data(), code.size()); }
    std::string GetCode() const { return std::string(code.data()); }

    std::array<char,20> code; ///< 用户唯一标识
};

/**
 * @struct Monitor_body
 * @brief 心跳/监控数据包体，包含内存使用率和后端地址信息
 */
struct Monitor_body : public packet_head
{
    Monitor_body() : packet_head()
    {
        cmd = Minotor;
        len = sizeof(Monitor_body);
        ip.fill('\0');
    }
    void SetIp(const std::string& str) { str.copy(ip.data(), ip.size()); }
    std::string GetIp() const { return std::string(ip.data()); }

    uint8_t mem;                 ///< 内存使用率百分比
    std::array<char,16> ip;      ///< 监控主机 IP
    uint16_t port;               ///< 监控主机端口
};

#pragma pack(pop)
#endif