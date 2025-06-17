/**
 * @file ECloudAssistant.cpp
 * @brief 实现主窗口 ECloudAssistant 的功能，包含标题栏、信息列表和主工作区的布局，并支持窗口拖拽
 * @author 
 * @date 2025-06-16
 */

#include "ECloudAssistant.h"
#include "TitleWgt.h"
#include "ListInfoWgt.h"
#include "MainWgt.h"
#include <QMouseEvent>
#include <QGridLayout>

ECloudAssistant::ECloudAssistant(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(800,540);
    setStyleSheet("background-color: #121212");

    mainWgt_ = new MainWgt(this);
    titleWgt_ = new TitleWgt(this);
    listWgt_ = new ListInfoWgt(this);

    connect(listWgt_,&ListInfoWgt::sig_Select,mainWgt_,&MainWgt::slot_ItemCliked);

    //布局
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->addWidget(listWgt_,0,0,2,1);
    layout->addWidget(titleWgt_,0,1,1,2);
    layout->addWidget(mainWgt_,1,1,1,2);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);
}

ECloudAssistant::~ECloudAssistant()
{
}

/**
 * @brief 鼠标移动事件处理函数，实现拖拽移动窗口
 * @param event 包含鼠标位置和按键信息的事件对象
 */
void ECloudAssistant::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton && is_press_)
    {
        if(!qobject_cast<QPushButton*>(childAt(event->pos())))
        {
            move(event->globalPos() - point_);
        }
    }
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief 鼠标按下事件处理函数，记录拖拽起点
 * @param event 包含鼠标按键信息的事件对象
 */
void ECloudAssistant::mousePressEvent(QMouseEvent *event)
{
    if(!qobject_cast<QPushButton*>(childAt(event->pos())))
    {
        is_press_ = true;
        point_ = event->globalPos() - this->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

/**
 * @brief 鼠标释放事件处理函数，结束拖拽状态
 * @param event 包含鼠标释放动作信息的事件对象
 */
void ECloudAssistant::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        is_press_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

