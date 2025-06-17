/**
 * @file MainWgt.h
 * @brief 定义主工作区控件 MainWgt，用于管理并切换多种子页面（登录、远程、设备、设置）
 * @date 2025-06-16
 */

#ifndef MAINWGT_H
#define MAINWGT_H

#include <QWidget>
#include <QStackedWidget>

class LoginWgt;
class RemoteWgt;

/**
 * @class MainWgt
 * @brief 主界面控件，使用 QStackedWidget 管理各子界面的切换
 */
class MainWgt : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化窗口属性并创建各子页面
     * @param parent 父控件指针，默认为 nullptr
     */
    explicit MainWgt(QWidget *parent = nullptr);

public slots:
    /**
     * @brief 响应列表项点击，根据索引切换对应的子页面
     * @param index 要切换到的页面索引
     */
    void slot_ItemCliked(int index);

private:
    QStackedWidget* stackWgt_; ///< 用于存放并切换各子页面的堆栈控件
    LoginWgt* login_;          ///< 登录界面控件
    RemoteWgt* remoteWgt_;     ///< 远程控制界面控件
    QWidget* deviceWgt_;       ///< 设备列表页面空白占位
    QWidget* settingWgt_;      ///< 高级设置页面空白占位
};

#endif // MAINWGT_H
