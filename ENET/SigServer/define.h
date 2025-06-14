#include <cstdint>
#include <array>
#include <string>

/**
 * @file define.h
 * @brief 定义信令服务器使用的命令、结果码、客户端状态及通信数据包结构。
 *
 * 本文件用于客户端与服务器之间的信令交互，支持房间创建、获取流、播放/停止流
 * 以及键鼠控制等功能。
 */

//需要1字节对齐
#pragma pack(push,1)

/**
 * @enum Cmd
 * @brief 客户端与服务器之间通信的命令标识符
 */
enum Cmd : uint16_t
{
    JOIN = 5,      ///< 加入房间或创建房间请求
    OBTAINSTREAM,  ///< 请求获取当前房间中可用的流信息
    CREATESTREAM,  ///< 请求创建新的媒体流
    PLAYSTREAM,    ///< 请求开始播放指定媒体流
    DELETESTREAM,  ///< 请求停止播放或删除流
    MOUSE,         ///< 鼠标点击事件
    MOUSEMOVE,     ///< 鼠标移动事件
    KEY,           ///< 键盘按键事件
    WHEEL,         ///< 鼠标滚轮事件
};

/**
 * @enum ResultCode
 * @brief 服务器对客户端请求的处理结果码
 */
enum ResultCode
{
    SUCCESSFUL,       ///< 操作成功
    ERROR,            ///< 操作失败
    REQUEST_TIMEOUT , ///< 请求超时
    ALREADY_REDISTERED,///< 已注册或重复操作
    USER_DISAPPEAR,   ///< 用户不存在或已断开
    ALREADY_LOGIN,    ///< 已登录
    VERFICATE_FAILED, ///< 验证失败
};

/**
 * @enum RoleState
 * @brief 客户端当前在房间中的状态
 */
enum RoleState
{
    IDLE,    ///< 已创建房间，但未推流也未拉流
    NONE,    ///< 未创建房间
    CLOSE,   ///< 客户端连接断开
    PULLER,  ///< 客户端处于拉流状态
    PUSHER,  ///< 客户端处于推流状态
};

/**
 * @struct packet_head
 * @brief 所有信令包的公共包头，包含包长度和命令类型
 */
struct packet_head
{
    packet_head():len(-1),cmd(-1){}
    int16_t len;  ///< 包体总长度（字节数）
    uint16_t cmd; ///< 命令类型，参考 Cmd 枚举
};

//包体
//创建房间
/**
 * @struct Join_body
 * @brief 加入房间或创建房间请求包体
 */
struct Join_body : public packet_head
{
    Join_body():packet_head()
    {
        cmd = JOIN;
        len = sizeof(Join_body);
        id.fill('\0');
    }
    /**
     * @brief 设置房间ID，长度最多10字节
     */
    void SetId(const std::string& str)
    {
        str.copy(id.data(), id.size());
    }
    /**
     * @brief 获取房间ID
     */
    std::string GetId() const
    {
        return std::string(id.data());
    }
    std::array<char,10> id; ///< 房间唯一标识字符串
};

/**
 * @struct JoinReply_body
 * @brief 加入房间请求的应答包，包含结果码
 */
struct JoinReply_body : public packet_head
{
    JoinReply_body():packet_head()
    {
        cmd = JOIN;
        len = sizeof(JoinReply_body);
        result = ERROR;
    }
    /**
     * @brief 设置操作结果码
     */
    void SetCode(ResultCode code)
    {
        result = code;
    }
    ResultCode result; ///< 操作返回的结果码
};
//获取流
/**
 * @struct ObtainStream_body
 * @brief 获取流信息请求包体
 */
struct ObtainStream_body : public packet_head
{
    ObtainStream_body() : packet_head()
    {
        cmd = OBTAINSTREAM;
        len = sizeof(ObtainStream_body);
        id.fill('\0');
    }
    void SetId(const std::string& str)
    {
        str.copy(id.data(),id.size(),0);
    }
    std::string GetId()
    {
        return std::string(id.data());
    }
    std::array<char,10> id;
};

/**
 * @struct ObtainStreamReply_body
 * @brief 获取流信息请求的应答包，包含结果码
 */
struct ObtainStreamReply_body : public packet_head
{
    ObtainStreamReply_body() : packet_head()
    {
        cmd = OBTAINSTREAM;
        len = sizeof(ObtainStreamReply_body);
        result = ERROR;
    }
    void SetCode(const ResultCode code)
    {
        result = code;
    }
    ResultCode result;
};
//创建流
/**
 * @struct CreateStream_body
 * @brief 创建流请求包体
 */
struct CreateStream_body : public packet_head
{
    CreateStream_body() : packet_head()
    {
        cmd = CREATESTREAM;
        len = sizeof(CreateStream_body);
    }
};

/**
 * @struct CreateStreamReply_body
 * @brief 创建流请求的应答包，包含流地址和结果码
 */
struct CreateStreamReply_body : public packet_head
{
    CreateStreamReply_body() : packet_head()
    {
        cmd = CREATESTREAM;
        len = sizeof(CreateStreamReply_body);
        result = ERROR;
        streamAddres.fill('\0');
    }
    void SetstreamAddres(const std::string& str)
    {
        str.copy(streamAddres.data(),streamAddres.size(),0);
    }
    std::string GetstreamAddres()
    {
        return std::string(streamAddres.data());
    }
    void SetCode(const ResultCode code)
    {
        result = code;
    }
    ResultCode result;
    std::array<char,70> streamAddres;  
};

//播放流 提供播放流地址
/**
 * @struct PlayStream_body
 * @brief 播放流请求包体
 */
struct PlayStream_body : public packet_head
{
    PlayStream_body() : packet_head()
    {
        cmd = PLAYSTREAM;
        len = sizeof(PlayStream_body);
        result = ERROR;
        streamAddres.fill('\0');
    }
    void SetstreamAddres(const std::string& str)
    {
        str.copy(streamAddres.data(),streamAddres.size(),0);
    }
    std::string GetstreamAddres()
    {
        return std::string(streamAddres.data());
    }
    void SetCode(const ResultCode code)
    {
        result = code;
    }
    ResultCode result;
    std::array<char,70> streamAddres;  
};

/**
 * @struct PlayStreamReplay_body
 * @brief 播放流请求的应答包
 */
struct PlayStreamReplay_body : public packet_head
{
    PlayStreamReplay_body() : packet_head()
    {
        cmd = PLAYSTREAM;
        len = sizeof(PlayStreamReplay_body);
        result = ERROR;
    }
    void SetCode(const ResultCode code)
    {
        result = code;
    }
    ResultCode result;
};

/**
 * @struct DeleteStream_body
 * @brief 删除流请求包体
 */
struct DeleteStream_body : public packet_head
{
    DeleteStream_body():packet_head()
    {
        cmd = DELETESTREAM;
        streamCount = -1;
        len = sizeof(DeleteStream_body);
    }
    void SetStreamCount(const int count)
    {
        streamCount = count;
    }
    int streamCount;//推流的时候，如果发现拉流数量为0,我们就需要停止推流，如果流数量不为0，就说明还有客户端连接，不能停止推流
};

//鼠标键盘信息
enum MouseType : uint8_t
{
    NoButton = 0,
    LeftButton = 1,
    RightButton = 2,
    MiddleButton = 4,
    XButton1 = 8,
    XButton2 = 16,
};

//鼠标键盘按下还是松开
enum MouseKeyType : uint8_t
{
    PRESS,
    RELESE,
};

//键盘消息
struct Key_body : public packet_head
{
    Key_body():packet_head(){
        cmd = KEY;
        len = sizeof(Key_body);
    }
    //键值和类型
    uint16_t key;
    MouseKeyType type;
};

//滚轮消息
struct Wheel_body : public packet_head
{
    Wheel_body():packet_head(){
        cmd = WHEEL;
        len = sizeof(Wheel_body);
    }
    //值
    uint8_t wheel;
};

//鼠标移动
struct MouseMove_body : public packet_head
{
    MouseMove_body():packet_head(){
        cmd = MOUSEMOVE;
        len = sizeof(MouseMove_body);
    }
    //需要x,y比值
    uint8_t xl_ratio;
    uint8_t xr_ratio;
    uint8_t yl_ratio;
    uint8_t yr_ratio;
};

struct Mouse_body : public packet_head
{
    Mouse_body():packet_head()
    {
        cmd = MOUSE;
        len = sizeof(Mouse_body);
    }
    MouseKeyType type;
    MouseType mouseType;
};

#pragma pack(pop)