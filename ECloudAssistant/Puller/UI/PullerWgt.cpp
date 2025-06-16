#include "PullerWgt.h"
#include <QResizeEvent>
#include <QVBoxLayout>

PullerWgt::PullerWgt(EventLoop* loop,QWidget *parent)
    : QMainWindow{parent}
{
    this->setMinimumSize(400,250);
    this->resize(800,500);
    //窗口标题
    setWindowTitle(QString("EcloudAssistant"));
    //窗口图标
    setWindowIcon(QIcon(":/UI/brown/center/favicon-32.ico"));
    //设置窗口背景
    setStyleSheet("background-color:#121212");

    player_.reset(new AVPlayer(loop,this));
    //布局
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(player_.get());
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    this->setLayout(layout);
}

bool PullerWgt::Connect(QString ip, uint16_t port, QString code)
{
    return player_->Connect(ip,port,code);
}

void PullerWgt::resizeEvent(QResizeEvent *event)
{
    //后面处理
    player_->resize(event->size());
    QMainWindow::resizeEvent(event);
}

