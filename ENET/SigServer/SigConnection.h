/**
 * @file SigConnection.h
 * @brief 信令服务器单个客户端连接处理类
 *
 * 继承自 TcpConnection，负责解析客户端信令协议数据包，
 * 支持以下主要功能：
 *   - 加入/创建房间（JOIN）
 *   - 获取可用流信息（OBTAINSTREAM）
 *   - 创建/删除流（CREATESTREAM、DELETESTREAM）
 *   - 播放流（PLAYSTREAM）
 *   - 键盘/鼠标控制转发（MOUSE、KEY、WHEEL）
 *
 * 管理客户端状态机（NONE、IDLE、PUSHER、PULLER、CLOSE），
 * 并维护与其他连接的关联关系，用于转发控制与流数据。
 */
class SigConnection : public TcpConnection
{
public:
    /**
     * @brief 构造函数，初始化连接状态并注册回调
     * @param scheduler 异步任务调度器
     * @param socket    已建立的 TCP 连接描述符
     */
    SigConnection(TaskScheduler* scheduler,int socket);

    /**
     * @brief 析构函数，自动清理关联资源
     */
    ~SigConnection();

    /** @brief 检查连接是否存活（未调用 DisConnected） */
    bool IsAlive() { return state_ != CLOSE; }
    /** @brief 检查是否未加入房间 */
    bool IsNoJion() { return state_ == NONE; }
    /** @brief 检查是否已加入且空闲（未推/拉流） */
    bool IsIdle() { return state_ == IDLE; }
    /** @brief 检查是否在推流或拉流中 */
    bool IsBusy() { return state_ == PUSHER || state_ == PULLER; }

    /**
     * @brief 处理客户端断开事件，清理状态并通知关联端
     */
    void DisConnected();

    /**
     * @brief 将指定客户端 ID 添加到关联列表，用于控制/转发
     * @param code 对方客户端唯一标识
     */
    void AddCustom(const std::string& code);

    /**
     * @brief 从关联列表移除指定客户端，并在无关联时恢复空闲状态
     * @param code 对方客户端唯一标识
     */
    void RmoveCustom(const std::string& code);

    /** @brief 获取当前客户端状态 */
    RoleState GetRoleState() const { return state_; }
    /** @brief 获取当前客户端的唯一标识 */
    std::string GetCode() const { return code_; }
    /** @brief 获取当前推流地址，用于提供给拉流端 */
    std::string GetStreamAddres() const { return streamAddres_; }

protected:
    /**
     * @brief TCP 数据到达回调，循环解析并处理所有完整消息
     * @param buffer 网络接收缓冲
     * @return 是否继续保持连接
     */
    bool OnRead(BufferReader& buffer);

    /**
     * @brief 解析单条信令消息并分发到具体处理函数
     * @param buffer 网络接收缓冲
     */
    void HandleMessage(BufferReader& buffer);

    /**
     * @brief 清理当前连接状态，将关联端通知删除流并移除自身注册
     */
    void Clear();

private:
    /** @brief 处理 JOIN（加入/创建房间）请求 */
    void HandleJion(const packet_head* data);
    /** @brief 处理 OBTAINSTREAM（获取流信息）请求 */
    void HandleObtainStream(const packet_head* data);
    /** @brief 处理 CREATESTREAM（创建流）请求 */
    void HandleCreateStream(const packet_head* data);
    /** @brief 处理 DELETESTREAM（删除/停止流）请求 */
    void HandleDeleteStream(const packet_head* data);
    /** @brief 处理其他控制消息并转发，如鼠标、键盘事件 */
    void HandleOtherMessage(const packet_head* data);

    /**
     * @brief 执行获取流逻辑，根据目标状态协商推流或播放
     * @param data 包头，实际体为 ObtainStream_body
     */
    void DoObtainStream(const packet_head* data);

    /**
     * @brief 执行创建流逻辑，并通知所有拉流端开始播放
     * @param data 包头，实际体为 CreateStreamReply_body
     */
    void DoCreateStream(const packet_head* data);

private:
    RoleState state_;                ///< 客户端状态机
    std::string code_;               ///< 客户端唯一标识（房间/用户 ID）
    std::string streamAddres_;       ///< 本地推流地址
    TcpConnection::Ptr conn_ = nullptr; ///< 关联的对方连接
    std::vector<std::string> objectes_; ///< 存储关联的其他客户端 ID 列表
};