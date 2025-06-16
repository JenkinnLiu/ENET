#include "MainWgt.h"
#include "RemoteWgt.h"
#include "LoginWgt.h"

MainWgt::MainWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,510);

    stackWgt_ = new QStackedWidget(this);
    stackWgt_->setFixedSize(600,510);
    login_ = new LoginWgt(this);
    remoteWgt_= new RemoteWgt(this);
    deviceWgt_= new QWidget(this);
    settingWgt_= new QWidget(this);

    deviceWgt_->setFixedSize(600,510);
    settingWgt_->setFixedSize(600,510);

    deviceWgt_->setStyleSheet("background-color: #664764");
    settingWgt_->setStyleSheet("background-color: #957522");

    stackWgt_->addWidget(login_); //0
    stackWgt_->addWidget(remoteWgt_); //1
    stackWgt_->addWidget(deviceWgt_); //2
    stackWgt_->addWidget(settingWgt_);//3

    connect(login_,&LoginWgt::sig_logined,remoteWgt_,&RemoteWgt::handleLogined);

    //指定显示哪个窗口
    stackWgt_->setCurrentWidget(remoteWgt_);
}

void MainWgt::slot_ItemCliked(int index)
{
    QWidget* widget = stackWgt_->widget(index);
    if(widget)
    {
        stackWgt_->setCurrentWidget(widget);
    }
}
