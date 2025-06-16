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

void ECloudAssistant::mousePressEvent(QMouseEvent *event)
{
    if(!qobject_cast<QPushButton*>(childAt(event->pos())))
    {
        is_press_ = true;
        point_ = event->globalPos() - this->frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(event);
}

void ECloudAssistant::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        is_press_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

