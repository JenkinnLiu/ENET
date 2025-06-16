#include "RemoteWgt.h"
#include <QVBoxLayout>
#include "StyleLoader.h"

RemoteWgt::RemoteWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,510);

    selfCodeEdit_ = new QLineEdit(this);
    rmoteCodeEdit_ = new QLineEdit(this);
    startRmoteBtn_ = new QPushButton(QString("开始远程"),this);
    manager_.reset(new RemoteManager());

    selfCodeEdit_->setText("345");

    this->setObjectName("RemoteWgt");
    selfCodeEdit_->setObjectName("selfCodeEdit");
    rmoteCodeEdit_->setObjectName("remoteCodeEdit");
    startRmoteBtn_->setObjectName("remoteBtn");

    selfCodeEdit_->setPlaceholderText(QString("本机识别码"));
    rmoteCodeEdit_->setPlaceholderText(QString("远程识别码"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addStretch(3);
    layout->addWidget(selfCodeEdit_,0,Qt::AlignCenter);
    layout->addSpacing(30);
    layout->addWidget(rmoteCodeEdit_,1,Qt::AlignCenter);
    layout->addWidget(startRmoteBtn_,2,Qt::AlignCenter);
    layout->addStretch(1);
    setLayout(layout);

    StyleLoader::getInstance()->loadStyle(":/UI/brown/main.css",this);

    connect(startRmoteBtn_,&QPushButton::clicked,this,[this](){
        //开始远程
        //获取远程code
        QString code = rmoteCodeEdit_->text();
        if(code.isEmpty())
        {
            return;
        }
        manager_->StartRemote(ip_,port_,code);
    });
}

void RemoteWgt::handleLogined(const std::string ip, uint16_t port)
{
    //code怎么来，按理说code应该是注册的时候由这个服务器通过算法来去创建分配，由客户端来创建。
    //连接信令服务器
    QString code = selfCodeEdit_->text();
    if(code.isEmpty())
    {
        return;
    }
    ip_ = QString(ip.c_str());
    port_ = port;
    manager_->Init(QString(ip.c_str()),port,code);
}
