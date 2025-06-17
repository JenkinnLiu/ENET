/**
 * @file MainWgt.cpp
 * @brief 实现主工作区控件 MainWgt，包括子页面创建、布局和切换逻辑
 * @date 2025-06-16
 */

#include "MainWgt.h"
#include "RemoteWgt.h"
#include "LoginWgt.h"

/**
 * @brief 构造函数，初始化无边框窗口并创建、布局登录、远程、设备与设置四个子页面
 * @param parent 父控件指针，默认为 nullptr
 */
MainWgt::MainWgt(QWidget *parent)
    : QWidget{parent}
{
    // 设置窗口属性
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,510);

    // 创建并配置 QStackedWidget，用于管理子页面
    stackWgt_ = new QStackedWidget(this);
    stackWgt_->setFixedSize(600,510);

    // 实例化各子页面控件
    login_ = new LoginWgt(this);
    remoteWgt_= new RemoteWgt(this);
    deviceWgt_= new QWidget(this);
    settingWgt_= new QWidget(this);

    // 设置占位页面的样式
    deviceWgt_->setFixedSize(600,510);
    settingWgt_->setFixedSize(600,510);
    deviceWgt_->setStyleSheet("background-color: #664764");
    settingWgt_->setStyleSheet("background-color: #957522");

    // 将子页面添加到堆栈中，对应索引 0~3
    stackWgt_->addWidget(login_);       ///< 登录页面
    stackWgt_->addWidget(remoteWgt_);   ///< 远程控制页面
    stackWgt_->addWidget(deviceWgt_);   ///< 设备列表页面
    stackWgt_->addWidget(settingWgt_);  ///< 高级设置页面

    // 登录成功后切换到远程控制页面
    connect(login_, &LoginWgt::sig_logined,
            remoteWgt_, &RemoteWgt::handleLogined);

    // 默认显示远程控制页面
    stackWgt_->setCurrentWidget(remoteWgt_);
}

/**
 * @brief 槽函数，响应列表项点击，根据索引切换对应子页面
 * @param index 子页面在 QStackedWidget 中的索引
 */
void MainWgt::slot_ItemCliked(int index)
{
    QWidget* widget = stackWgt_->widget(index);
    if(widget)
    {
        stackWgt_->setCurrentWidget(widget);
    }
}
