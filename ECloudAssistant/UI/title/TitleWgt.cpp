/**
 * @file TitleWgt.cpp
 * @brief 自定义窗口标题栏实现，包含最小化和关闭按钮的布局和行为
 * @date 2025-06-16
 */

#include "TitleWgt.h"
#include <QHBoxLayout>
#include "StyleLoader.h"

/**
 * @brief 构造函数，初始化无边框标题栏并创建按钮，设置布局和样式
 * @param parent 父窗口指针
 */
TitleWgt::TitleWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,30);

    minBtn_ = new QPushButton(this);
    closeBtn_ = new QPushButton(this);
    minBtn_->setFixedSize(30,30);
    closeBtn_->setFixedSize(30,30);

    closeBtn_->setObjectName("close_Btn");
    minBtn_->setObjectName("min_Btn");

    connect(minBtn_,&QPushButton::clicked,this,[this](){
        this->parentWidget()->showMinimized();
    });

    connect(closeBtn_,&QPushButton::clicked,this,[this](){
        this->parentWidget()->close();
    });

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addStretch(1);
    layout->addWidget(minBtn_);// 添加最小化按钮
    layout->addWidget(closeBtn_); // 添加关闭按钮
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    // 设置样式表
    StyleLoader::getInstance()->loadStyle(":/UI/brown/main.css", this);
}
