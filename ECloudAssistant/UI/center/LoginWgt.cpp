#include "LoginWgt.h"
#include <QVBoxLayout>
#include "StyleLoader.h"
#include <QDateTime>

uint64_t GetTimeStamp()
{
    QDateTime currentDataTime = QDateTime::currentDateTime();
    qint64 timestamp = currentDataTime.toSecsSinceEpoch();
    return static_cast<uint64_t>(timestamp);
}

LoginWgt::LoginWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,510);

    acountEdit_ = new QLineEdit(this);
    passwordEdit_ = new QLineEdit(this);

    acountEdit_->setText("17378161017");
    passwordEdit_->setText("12345678");

    loginBtn_ = new QPushButton(QString("登录"),this);
    connect(loginBtn_,&QPushButton::clicked,this,[this](){
        if(socket_ && is_connect_)
        { //请求负载返回登陆服务器IP和端口去开始登录
            Login_Info info;
            info.timestamp = GetTimeStamp();
            socket_->write((const char*)&info,info.len);
            socket_->flush();
        }
    });

    //创建套接字
    socket_ = new QTcpSocket(this);
    //关联信号与槽
    connect(socket_,SIGNAL(readyRead()),this,SLOT(ReadData()));
    //连接负载
    socket_->connectToHost("192.168.31.30",8523);
    if(socket_->waitForConnected(1000))
    {
        is_connect_ = true;
        Login_Info info;
        info.timestamp = GetTimeStamp();
        socket_->write((const char*)&info,info.len);
        socket_->flush();
        qDebug() << "连接负载成功";
    }

    this->setObjectName("Loginer");
    acountEdit_->setObjectName("AcountEdit");
    passwordEdit_->setObjectName("PasswdEdit");
    loginBtn_->setObjectName("login_Btn");

    acountEdit_->setPlaceholderText(QString("请输入账号"));
    passwordEdit_->setPlaceholderText(QString("请输入密码"));
    passwordEdit_->setEchoMode(QLineEdit::Password);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addStretch(3);
    layout->addWidget(acountEdit_,0,Qt::AlignCenter);
    layout->addSpacing(30);
    layout->addWidget(passwordEdit_,1,Qt::AlignCenter);
    layout->addWidget(loginBtn_,2,Qt::AlignCenter);
    layout->addStretch(1);
    setLayout(layout);

    StyleLoader::getInstance()->loadStyle(":/UI/brown/main.css",this);
}

void LoginWgt::ReadData()
{
    QByteArray buf = socket_->readAll();
    if(!buf.isEmpty())
    {
        HandleMessage((packet_head*)buf.data());
    }
}

void LoginWgt::HandleMessage(const packet_head *data)
{
    switch (data->cmd) {
    case Login:
        HandleLogin((LoginResult*)data);
        break;
    case Register:
        HandleRegister((RegisterResult*)data);
        break;
    case ERROR_:
        HandleError((packet_head*)data);
        break;
    default:
        break;
    }
}

void LoginWgt::HandleRegister(RegisterResult *data)
{

}

void LoginWgt::HandleLogin(LoginResult *data)
{
    //登录有两个回应 一个是负载返回登录结果/返回一个登陆服务器ip，port,在是一个登陆服务器，返回信令服务器ip,POrt
    if(is_login_) //这是登陆服务器返回 返回信令服务器IP port
    {
        if(data->resultCode == S_OK_) //登录成功，获取信令服务器ip,port
        {
            std::string sigserver = data->GetIp();
            qDebug() << "login succeful " << "sigIp: " << sigserver.c_str() << "port: " << data->port;
            emit sig_logined(data->GetIp(),data->port);
        }
    }
    else //这是负载均衡返回的登陆服务器IP port 去登陆
    {
        HandleLoadLogin((LoginReply*)data);
    }
}

void LoginWgt::HandleError(const packet_head *data)
{
    qDebug() << "error";
}

void LoginWgt::HandleLoadLogin(LoginReply *data)
{
    //请求负载，返回的结果
    ip_ = QString(data->ip.data());
    port_  = data->port;
    qDebug() << "login ip: " << ip_ << "port: " << port_;

    //获取到登陆服务器
    //断开负载连接登录服务
    socket_->disconnectFromHost();
    is_connect_ = false;

    socket_->connectToHost(ip_,port_);
    if(socket_->waitForConnected(1000))
    {
        is_connect_ = true;
        is_login_ = true;

        qDebug() << "登录服务器";

        //开始登录
        UserLogin login;
        QString acount = acountEdit_->text();
        QString passwd = passwordEdit_->text();
        login.SetCode("345");
        login.SetCount(acount.toStdString());
        login.SetPasswd(passwd.toStdString());
        login.timestamp = GetTimeStamp();
        socket_->write((const char*)&login,login.len);
        socket_->flush();
    }
}
