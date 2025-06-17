/**
 * @file LoginWgt.cpp
 * @brief 实现 LoginWgt 类，管理用户登录界面与网络交互逻辑
 * @date 2025-06-17
 */

/**
 * @brief 获取当前时间戳（秒级）
 * @return uint64_t 当前 Unix 时间戳
 */
uint64_t GetTimeStamp()
{
    QDateTime currentDataTime = QDateTime::currentDateTime();
    qint64 timestamp = currentDataTime.toSecsSinceEpoch();
    return static_cast<uint64_t>(timestamp);
}

/**
 * @brief 构造函数，初始化登录界面并尝试连接负载均衡服务器
 * @param parent 父控件指针
 */
// LoginWgt 类的构造函数实现
LoginWgt::LoginWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,510);

    acountEdit_ = new QLineEdit(this);
    passwordEdit_ = new QLineEdit(this);

    acountEdit_->setText("17378161017");// 设置默认账号
    passwordEdit_->setText("12345678"); // 设置默认密码

    loginBtn_ = new QPushButton(QString("登录"),this); // 创建登录按钮
    connect(loginBtn_,&QPushButton::clicked,this,[this](){
        if(socket_ && is_connect_)
        { //请求负载返回登陆服务器IP和端口去开始登录
            Login_Info info;
            info.timestamp = GetTimeStamp();
            socket_->write((const char*)&info,info.len);// 发送登录请求到负载均衡服务器
            socket_->flush();
        }
    }); // 连接登录按钮点击信号到槽函数

    //创建套接字
    socket_ = new QTcpSocket(this);
    //关联信号与槽
    connect(socket_,SIGNAL(readyRead()),this,SLOT(ReadData()));
    //连接负载均衡服务器
    socket_->connectToHost("192.168.31.30",8523); // 连接到负载均衡服务器
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

    // 设置布局
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

/**
 * @brief 槽函数，读取网络数据并调用 HandleMessage 解析消息包
 */
void LoginWgt::ReadData()
{
    QByteArray buf = socket_->readAll();
    if(!buf.isEmpty())
    {
        HandleMessage((packet_head*)buf.data());
    }
}

/**
 * @brief 根据命令类型分发到具体处理函数
 * @param data 指向收到的消息包头
 */
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

/**
 * @brief 处理注册结果回调（预留接口）
 * @param data 注册结果包指针
 */
void LoginWgt::HandleRegister(RegisterResult *data)
{

}

/**
 * @brief 处理登录结果包，包括负载均衡服务器和登录服务器两种情况
 * @param data 登录结果包指针
 */
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

/**
 * @brief 处理错误消息包
 * @param data 错误包头指针
 */
void LoginWgt::HandleError(const packet_head *data)
{
    qDebug() << "error";
}

/**
 * @brief 处理负载均衡服务器返回的登录信息，并连接登录服务器
 * @param data 登录应答包指针
 */
void LoginWgt::HandleLoadLogin(LoginReply *data)
{
    //请求负载，返回的结果
    ip_ = QString(data->ip.data());
    port_  = data->port;
    qDebug() << "login ip: " << ip_ << "port: " << port_;

    //获取到登陆服务器
    //断开负载连接登录服务
    socket_->disconnectFromHost();// 断开负载均衡服务器连接
    is_connect_ = false;

    socket_->connectToHost(ip_,port_);// 连接到登录服务器
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
