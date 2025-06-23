/**
 * @file SigConnection.cpp
 * @brief 实现了客户端信令连接类 SigConnection 的功能。
 * @details
 * 该文件包含了与信令服务器交互的完整逻辑实现，包括：
 * - 连接建立后的初始化和回调设置。
 * - 接收和解析来自服务器的数据包。
 * - 根据不同的命令（如 JOIN, PLAYSTREAM, MOUSE 等）执行相应的状态转换和操作。
 * - 作为控制端时，主动发起获取流的请求。
 * - 作为被控端时，响应创建流的请求，并模拟执行远程的鼠标和键盘事件。
 */
#include "SigConnection.h"
#include "defin.h"
#include <QDebug>
#include <windows.h>
#include <QGuiApplication>

/// 全局变量，用于生成递增的流ID，确保每次推流地址唯一。
int streamIndex = 1;

/**
 * @brief 构造函数
 * @param scheduler 任务调度器指针
 * @param sockfd TCP套接字文件描述符
 * @param code 识别码
 * @param type 用户类型 (控制端/被控端)
 */
SigConnection::SigConnection(TaskScheduler *scheduler, int sockfd, const QString& code,const UserType &type)
    :TcpConnection(scheduler,sockfd)
    ,state_(NONE)
    ,type_(type)
    ,code_(code)
{
    // 设置基类TcpConnection的读事件回调，当有数据可读时，调用本类的OnRead方法
    SetReadCallback([this](std::shared_ptr<TcpConnection> conn,BufferReader& buffer){
        return this->OnRead(buffer);
    });
    // 设置基类TcpConnection的关闭事件回调，当连接断开时，调用本类的OnClose方法
    SetCloseCallback([this](std::shared_ptr<TcpConnection> conn){
        return this->OnClose();
    });

    // 获取主屏幕对象，用于后续获取屏幕分辨率
    screen_ = QGuiApplication::primaryScreen();
    // 连接建立后，立即发送加入房间的请求
    Join();
}

/**
 * @brief 析构函数
 */
SigConnection::~SigConnection()
{

}

/**
 * @brief 读事件处理函数
 * @param buffer 包含网络数据的读缓冲区
 * @return 总是返回true，表示数据已处理
 * @details 循环处理缓冲区中的数据，直到所有数据都被解析成消息包。
 */
bool SigConnection::OnRead(BufferReader &buffer)
{
    while(buffer.ReadableBytes() > 0)
    {
        HandleMessage(buffer);
    }
    return true;
}

/**
 * @brief 连接关闭处理函数
 * @details 当TCP连接断开时，此函数被调用，设置退出标志。
 */
void SigConnection::OnClose()
{
    quit_ = true;
}

/**
 * @brief 消息分发函数
 * @param buffer 包含网络数据的读缓冲区
 * @details
 * 1. 尝试从缓冲区中窥探一个完整的消息包头(packet_head)。
 * 2. 检查缓冲区中的数据是否足够一个完整的消息包。
 * 3. 如果数据完整，则根据消息头中的命令类型(cmd)调用相应的do...()处理函数。
 * 4. 处理完毕后，从缓冲区中移除已处理的消息包数据。
 */
void SigConnection::HandleMessage(BufferReader &buffer)
{
    // 窥探消息头，以获取消息总长度
    packet_head* head = (packet_head*)buffer.Peek();
    // 如果缓冲区可读字节数小于消息长度，说明数据包不完整，直接返回等待更多数据
    if(buffer.ReadableBytes() < head->len)
    {
        return ; // 数据未收全，不做任何操作
    }
    // 数据包完整，根据命令类型进行分发处理
    switch (head->cmd) {
    case JOIN:
        doJoin(head); // 处理加入房间的响应
        break;
    case PLAYSTREAM:
        doPlayStream(head); // 处理播放流的指令
        break;
    case CREATESTREAM:
        doCtreatStream(head); // 处理创建流的请求
        break;
    case DELETESTREAM:
        doDeleteStream(head); // 处理删除流的指令
        break;
    case MOUSE:
        doMouseEvent(head); // 处理鼠标点击事件
        break;
    case MOUSEMOVE:
        doMouseMoveEvent(head); // 处理鼠标移动事件
        break;
    case KEY:
        doKeyEvent(head); // 处理键盘事件
        break;
    case WHEEL:
        doWheelEvent(head); // 处理鼠标滚轮事件
        break;
    default:
        break;
    }
    // 从缓冲区移除已经处理过的数据包
    buffer.Retrieve(head->len);
}

/**
 * @brief 发送加入房间请求
 * @return 成功返回0，如果状态不正确则返回-1
 * @details 构造一个Join_body消息包，并将其发送到信令服务器。
 */
qint32 SigConnection::Join()
{
    // 只有在初始状态(NONE)下才能发送加入房间请求
    if(state_ != NONE)
    {
        return -1;
    }
    // 申请创建房间
    Join_body body;
    if(type_ == CONTROLLED)
    {
        body.SetId(code_.toStdString());//设置识别码，由外部传入
    }
    else//控制端，就需要重新创建一个匿名code
    {
        body.SetId("154564");//设置识别码，由外部传入
    }
    this->Send((const char*)&body,body.len);
}

/**
 * @brief (仅控制端) 发送获取流请求
 * @return 成功返回0，如果状态或身份不正确则返回-1
 * @details 构造一个ObtainStream_body消息包，并将其发送到信令服务器。
 */
qint32 SigConnection::obtainStream()
{
    // 只有在空闲状态(IDLE)且身份是控制端(CONTROLLING)时才能获取流
    if(state_ == IDLE && type_ == CONTROLLING)
    {
        //获取流
        ObtainStream_body body;
        body.SetId(code_.toStdString());//id就是标识符，每个客户端有一个标识符
        this->Send((const char*)&body,body.len);
        return 0;
    }
    return -1;
}

/**
 * @brief 处理加入房间的响应
 * @param data 指向服务器响应的数据包
 * @details
 * 1. 解析服务器的响应结果。
 * 2. 如果成功，将状态更新为IDLE。
 * 3. 如果自身是控制端，则立即发送获取流的请求。
 */
void SigConnection::doJoin(const packet_head* data)
{
    // 将通用消息头转换为JoinReply_body类型
    JoinReply_body* reply = (JoinReply_body*)data;
    if(reply->result == S_OK)
    {
        // 加入成功，更新状态为空闲
        state_ = IDLE;
        // 如果是控制端，则在加入成功后立即请求获取流
        if(type_ == CONTROLLING)
        {
            //获取流
            if(obtainStream() != 0)
            {
                qDebug() << "获取流请求发送失败";
            }
            else
            {
                state_ = PULLER; // 发送请求后，状态变为等待拉流
                qDebug() << "获取流请求发送成功";
            }
        }
    }
    // 如果是被控端，则加入成功后进入IDLE状态，等待服务器指令

}

/**
 * @brief (仅控制端) 处理播放流指令
 * @param data 指向服务器发来的PlayStream_body数据包
 * @details
 * 1. 检查自身状态和身份是否正确。
 * 2. 如果服务器同意播放，则从数据包中提取流地址。
 * 3. 调用上层注册的startStreamCb_回调函数，将流地址传递给播放器模块，启动播放。
 */
void SigConnection::doPlayStream(const packet_head* data)
{
    // 只有在拉流状态(PULLER)且身份是控制端(CONTROLLING)时才处理此消息
    if(state_ == PULLER && type_ == CONTROLLING)
    {
        //开始播放流
        PlayStream_body* playStream = (PlayStream_body*)data;
        if(playStream->result == S_OK)
        {
            // 服务器同意播放，通过回调函数将流地址传给上层业务逻辑
            qDebug() << "开始播放流";
            if(startStreamCb_)
            {
                startStreamCb_(QString::fromStdString(playStream->GetstreamAddres()));
            }
        }
        else
        {
            qDebug() << "播放流失败";
        }
    }

}

/**
 * @brief (仅被控端) 处理创建流请求
 * @param data 指向服务器发来的CreateStream_body数据包
 * @details
 * 1. 检查自身状态和身份是否正确。
 * 2. 生成一个唯一的RTMP推流地址。
 * 3. 调用上层注册的startStreamCb_回调函数，将流地址传递给推流器模块，启动推流。
 * 4. 如果推流成功，将流地址和成功状态回复给服务器；否则回复失败状态。
 */
void SigConnection::doCtreatStream(const packet_head* data)
{
    // 只有在空闲状态(IDLE)且身份是被控端(CONTROLLED)时才处理此消息
    if(state_ == IDLE && type_ == CONTROLLED)
    {
        CreateStreamReply_body reply;
        // 准备一个本地推流地址，使用全局streamIndex保证唯一性
        QString streamAddr = "rtmp://192.168.31.30:1935/live/" + QString::number(++streamIndex);
        // 开始推流
        if(startStreamCb_)
        {
            // 调用回调，通知上层业务逻辑（推流器）开始推流
            if(startStreamCb_(streamAddr))
            {
                // 推流成功，构造回复消息，包含流地址和成功码
                reply.SetstreamAddres(streamAddr.toStdString());
                reply.SetCode((ResultCode)0);
                // 发送回复给服务器
                qDebug() << "streamaddr: " << reply.GetstreamAddres().c_str() << "len: " << reply.len;
                this->Send((const char*)&reply,reply.len);
                state_ = PUSHER; // 更新状态为正在推流
            }
            else
            {
                // 推流失败，构造回复消息，包含错误码
                qDebug() << "streamaddr failed: ";
                reply.SetCode(SERVER_ERROR);
                this->Send((const char*)&reply,reply.len);
            }
        }
    }
}

/**
 * @brief 处理删除流指令
 * @param data 指向服务器发来的DeleteStream_body数据包
 * @details 当服务器通知没有客户端在拉此路流时，调用停止回调函数来停止本地推流。
 */
void SigConnection::doDeleteStream(const packet_head* data)
{
    // 如果推流端发现这个拉流数量为0,我们就需要停止推流
    DeleteStream_body* body = (DeleteStream_body*)data;
    if(body->streamCount == 0)
    {
        // 调用回调，通知上层业务逻辑（推流器）停止推流
        if(stopStreamCb_)
        {
            stopStreamCb_();
        }
    }
}

/**
 * @brief (仅被控端) 处理鼠标点击事件
 * @param data 指向Mouse_Body数据包
 * @details 解析鼠标按键（左/中/右）和动作（按下/释放），并使用Windows API SendInput来模拟本地鼠标点击。
 */
void SigConnection::doMouseEvent(const packet_head *data)
{
    //处理鼠标按下松开事件
    Mouse_Body* body = (Mouse_Body*)data;
    DWORD dwFlags = 0;
    // 根据消息中的按钮和类型，组合MOUSEEVENTF_* 标志位
    if(body->type == PRESS)
    {
        dwFlags |= (body->mouseButtons & MouseType::LeftButton) ? MOUSEEVENTF_LEFTDOWN : 0;
        dwFlags |= (body->mouseButtons & MouseType::RightButton) ? MOUSEEVENTF_RIGHTDOWN : 0;
        dwFlags |= (body->mouseButtons & MouseType::MiddleButton) ? MOUSEEVENTF_MIDDLEDOWN : 0;
    }
    else if(body->type == RELEASE)
    {
        dwFlags |= (body->mouseButtons & MouseType::LeftButton) ? MOUSEEVENTF_LEFTUP : 0;
        dwFlags |= (body->mouseButtons & MouseType::RightButton) ? MOUSEEVENTF_RIGHTUP : 0;
        dwFlags |= (body->mouseButtons & MouseType::MiddleButton) ? MOUSEEVENTF_MIDDLEUP : 0;
    }
    // 模拟事件
    if(dwFlags != 0)
    {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = dwFlags;
        SendInput(1,&input,sizeof(input));// 发送输入事件
    }
}

/**
 * @brief (仅被控端) 处理鼠标移动事件
 * @param data 指向MouseMove_Body数据包
 * @details
 * 1. 从数据包中解析出鼠标位置在远程屏幕上的X,Y坐标比例。
 * 2. 将比例乘以本地屏幕的宽高，计算出在本地屏幕上对应的绝对坐标。
 * 3. 使用QCursor::setPos()将本地鼠标移动到该位置。
 */
void SigConnection::doMouseMoveEvent(const packet_head *data)
{
    // 鼠标移动
    // 接收到的坐标是相对于远程屏幕宽高的比例值
    MouseMove_Body* body = (MouseMove_Body*)data;
    // 将整数和小数部分组合成完整的浮点数比例
    double combined_x = (static_cast<double>(body->xl_ratio) + (static_cast<double>(body->xr_ratio) / 100.0f)) / 100.0f;
    double combined_y = (static_cast<double>(body->yl_ratio) + (static_cast<double>(body->yr_ratio) / 100.0f)) / 100.0f;
    // 将比例乘以本地屏幕的尺寸，得到本地的绝对坐标
    int x = static_cast<int>(combined_x * screen_->size().width() / screen_->devicePixelRatio());
    int y = static_cast<int>(combined_y * screen_->size().height() / screen_->devicePixelRatio());
    // 使用Qt的QCursor设置鼠标位置
    QCursor::setPos(x,y);
}

/**
 * @brief (仅被控端) 处理键盘事件
 * @param data 指向Key_Body数据包
 * @details 解析按键的虚拟键码(Virtual Key)和动作（按下/释放），并使用Windows API SendInput来模拟本地键盘输入。
 */
void SigConnection::doKeyEvent(const packet_head *data)
{
    //获取键值去模拟
    Key_Body* body = (Key_Body*)data;
    DWORD vk = body->key;
    qDebug() << "key: " << vk;
    INPUT input[1];
    ZeroMemory(input,sizeof(input));
    // 根据消息中的类型，设置KEYEVENTF_KEYUP标志位（0代表按下）
    int op = body->type ? KEYEVENTF_KEYUP : 0;
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = vk;
    input[0].ki.dwFlags = op;
    input[0].ki.wScan = MapVirtualKey(vk,0);
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;
    SendInput(1,input,sizeof(input));
}

/**
 * @brief (仅被控端) 处理鼠标滚轮事件
 * @param data 指向Wheel_Body数据包
 * @details 解析滚轮滚动方向和幅度，并使用Windows API SendInput来模拟本地鼠标滚轮滚动。
 */
void SigConnection::doWheelEvent(const packet_head *data)
{
    Wheel_Body* body = (Wheel_Body*)data;
    //获取滚轮是向上还是向下滚动
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = body->wheel * 240; // 滚动幅度，通常120的倍数
    SendInput(1,&input,sizeof(input));
}
