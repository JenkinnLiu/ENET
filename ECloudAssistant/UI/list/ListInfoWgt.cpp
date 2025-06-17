/**
 * @file ListInfoWgt.cpp
 * @brief 实现侧边栏列表控件 ListInfoWgt，包含用户按钮和功能项列表的布局逻辑
 * @date 2025-06-17
 */

#include "ListInfoWgt.h"
#include <QVBoxLayout>
#include "ListWidget.h"
#include "CustomWgt.h"
#include "StyleLoader.h"

// 列表信息窗口部件实现
ListInfoWgt::ListInfoWgt(QWidget *parent)
    : QWidget{parent}
{
    // 设置无边框和背景
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(200,540);

    // 创建用户按钮和列表控件
    userBtn_ = new QPushButton(this);
    listWgt_ = new ListWidget(this);

    // 配置对象名用于样式表
    userBtn_->setObjectName("user_Btn");
    listWgt_->setObjectName("listWidget");

    // 连接列表点击信号到处理槽
    connect(listWgt_, &ListWidget::itemCliked,
            this, &ListInfoWgt::HandleItemSelect);

    // 用户按钮点击处理，重置高亮并发出选择信号
    connect(userBtn_, &QPushButton::clicked, this, [this]() {
        emit sig_Select(0);
        for (auto item : customWgts_) {
            //取消高亮
            item->setHightLight(false);
        }
        listWgt_->clearSelection();
    });

    // 创建并初始化功能项控件，设置图片与文本
    CustomWgt* rmtWgt = new CustomWgt(this);
    CustomWgt* dvcWgt = new CustomWgt(this);
    CustomWgt* setWgt = new CustomWgt(this);

    rmtWgt->setImageAndText("远程控制",":/UI/brown/list/remote.png",
                           ":/UI/brown/list/remote_press.png", true);
    dvcWgt->setImageAndText("设备列表",":/UI/brown/list/device.png",
                            ":/UI/brown/list/device_press.png");
    setWgt->setImageAndText("高级设置",":/UI/brown/list/setting.png",
                            ":/UI/brown/list/setting_press.png");

    // 存储功能项并添加到列表控件
    customWgts_.push_back(rmtWgt);// 添加远程控制项
    customWgts_.push_back(dvcWgt);// 添加设备列表项
    customWgts_.push_back(setWgt);// 添加高级设置项

    listWgt_->AddWidget(rmtWgt);// 添加远程控制项
    listWgt_->AddWidget(dvcWgt);// 添加设备列表项
    listWgt_->AddWidget(setWgt);// 添加高级设置项


    // 设置控件大小并布局
    userBtn_->setFixedSize(100,100);
    listWgt_->setFixedSize(200,340);

    //布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addSpacing(50);
    layout->addWidget(userBtn_, 0, Qt::AlignHCenter);
    layout->addSpacing(50);
    layout->addWidget(listWgt_, 1, Qt::AlignHCenter);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    // 加载样式表
    StyleLoader::getInstance()->loadStyle(":/UI/brown/main.css", this);
}

/**
 * @brief 槽函数，处理列表项选中事件，更新对应项高亮并发出选择信号
 * @param index 被点击的列表项索引
 */
void ListInfoWgt::HandleItemSelect(int index) //0
{
    if(index < 0 || (index > customWgts_.size() - 1))
    {
        return;
    }
    emit sig_Select(index + 1);
    for(int i = 0; i < customWgts_.size();i++)
    {
        if(i == index)
        {
            //设置高亮
            customWgts_[i]->setHightLight(true);
        }
        else
        {
            customWgts_[i]->setHightLight(false);
        }
    }
}
