/**
 * @file LoginWgt.h
 * @brief 登录界面控件，负责与服务器交互完成用户登录流程，并在登录成功后发出信号
 * @date 2025-06-17
 */

#ifndef LOGINWGT_H
#define LOGINWGT_H
#include <QTcpSocket>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
#include "defin.h"

/**
 * @class LoginWgt
 * @brief 用户登录控件，包含账号输入、密码输入及登录按钮，处理网络通信逻辑
 */
class LoginWgt : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化登录界面并创建网络连接
     * @param parent 父控件指针，默认为 nullptr
     */
    explicit LoginWgt(QWidget *parent = nullptr);
signals:
    /**
     * @brief 登录成功信号，通知主界面连接信令服务器
     * @param ip 登录结果返回的信令服务器 IP 地址
     * @param port 登录结果返回的信令服务器端口
     */
    void sig_logined(const std::string ip, uint16_t port);
protected slots:
    /**
     * @brief 槽函数，读取来自服务器的数据并转发处理
     */
    void ReadData();
    /**
     * @brief 解析接收到的网络包，并分发到相应处理函数
     * @param data 指向网络包头部的指针
     */
    void HandleMessage(const packet_head* data);
protected:
    /**
     * @brief 处理注册结果的回调（暂未实现）
     * @param data 指向注册结果包的指针
     */
    void HandleRegister(RegisterResult* data);
    /**
     * @brief 处理登录结果，根据不同阶段区分负载服务器与登录服务器
     * @param data 指向登录结果包的指针
     */
    void HandleLogin(LoginResult* data);
    /**
     * @brief 处理错误包
     * @param data 指向错误包头的指针
     */
    void HandleError(const packet_head* data);
    /**
     * @brief 处理负载服务器返回的登录信息包，连接后续登录服务器
     * @param data 指向负载登录应答包的指针
     */
    void HandleLoadLogin(LoginReply* data);
private:
    QLineEdit* acountEdit_;      ///< 账号输入框
    QLineEdit* passwordEdit_;    ///< 密码输入框
    QPushButton* loginBtn_;      ///< 登录按钮
private:
    QString ip_;                 ///< 登录服务器 IP
    uint16_t port_;              ///< 登录服务器端口
    bool is_login_ = false;      ///< 是否已连接登录服务器
    bool is_connect_ = false;    ///< 是否已连接负载服务器
    QTcpSocket* socket_ = nullptr;///< 网络套接字，用于通信
};

#endif // LOGINWGT_H
