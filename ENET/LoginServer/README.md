LoginServer 是提供用户注册、登录和注销服务的独立 TCP 应用，其核心模块和处理流程如下：

1. 协议定义（define.h）  
   - Cmd 枚举：Register、Login、Destory（三种业务命令）  
   - ResultCode 枚举：S_OK、SERVER_ERROR、REQUEST_TIMEOUT、ALREADY_REDISTERED、ALREADY_LOGIN…  
   - packet_head + 派生的 UserRegister/UserLogin/UserDestory 请求结构，以及 RegisterResult/LoginResult 应答结构  
   - 使用 `#pragma pack(1)` 保证网络字节对齐  

2. 持久化层（ORMManager）  
   - 单例模式（`std::call_once`）+ MySQL C-API 连接  
   - 提供 `UserRegister`（INSERT 新用户）、`UserLogin`（SELECT 用户记录）和 `UserDestroy`（DELETE 用户）接口  
   - LoginConnection 通过它完成所有数据库增删查操作  

3. 网络层  
   - LoginServer（继承自 TcpServer）  
     • static Create(EventLoop*): 构造并启动监听  
     • OnConnect(socket): 每来一个新连接，创建一个 LoginConnection 实例  

4. 会话处理（LoginConnection）  
   - 构造时注册了 `SetReadCallback` → OnRead(BufferReader&)  
   - OnRead 中调用 `HandleMessage`，从 `BufferReader` 解析出一个完整包（通过 `packet_head::len`）后分发：  
     • Register → `HandleRegister`  
     • Login    → `HandleLogin`  
     • Destory  → `HandleDestory`  
   - 各 Handler 流程：  
     1) **超时检测**：请求包中带 `timestamp`，与当前系统秒级时间比较，超时直接返回 `REQUEST_TIMEOUT`  
     2) **数据库操作**：调用 ORMManager  
       - 注册：先查无记录再插入，否则返回 `ALREADY_REDISTERED`  
       - 登录：查不到返回 `SERVER_ERROR`，已登录返回 `ALREADY_LOGIN`，否则更新在线状态并返回 `S_OK` + 信令服务器 IP/端口  
       - 注销：直接删除用户记录  
     3) **构造应答**：填充 `RegisterResult` / `LoginResult`，通过 `Send(...)` 一次写回客户端  
     4) **缓冲回收**：`buffer.Retrieve(head->len)` 丢弃已消费数据  

5. 超时和资源清理  
   - 通过 `TIMEOUT` 宏（60 s）保证请求不过期  
   - LoginConnection 析构时会调用 `Clear()`（可扩展）释放资源  

6. 运行时架构  
   - 由主函数创建 `EventLoop` → `LoginServer::Create(loop)` → 启动监听  
   - `EventLoop` 在内部管理 epoll/timer，TcpServer 接收连接并调度到 LoginConnection  
   - 所有 I/O 完全异步，数据库操作同步阻塞（可按需改造为线程池或异步）  

总结：  
LoginServer 将“网络 I/O → 包解析 → 超时校验 → 数据库 CRUD → 应答发送”流程串成一条简洁的管道，通过单例 ORMManager 统一管理 MySQL 连接，用小巧的协议结构完成用户管理功能。


ORMManager 是登录服务器中负责用户信息持久化操作的核心单例类，封装了mysql的增删改查api，主要职责与特点包括：

1. 单例模式，线程安全  
   - 私有构造与析构，禁止外部直接 new/delete  
   - 静态成员 `instance_` 保存唯一实例  
   - `GetInstance()` 用 `std::call_once` + `std::once_flag` 保证多线程环境下只创建一次  

2. MySQL 连接管理  
   - 构造函数中通过 `mysql_init` 与 `mysql_real_connect` 建立 TCP 连接到数据库  
   - 析构函数中调用 `mysql_close` 断开并释放资源  
   - 单个连接句柄 `MYSQL mysql_` 贯穿整个应用生命周期  

3. 对外接口：增删查  
   - `void UserRegister(...)`  
     • 获取当前时间戳（秒）  
     • 调用私有方法 `insertClient` 向 `clients` 表插入一条新记录  
   - `MYSQL_ROW UserLogin(const char* usercode)`  
     • 调用私有方法 `selectClientByUsercode` 执行 `SELECT * FROM clients WHERE USER_CODE=?`  
     • 返回第一行记录或 NULL  
   - `void UserDestroy(const char* usercode)`  
     • 调用私有方法 `deleteClientByUsercode` 执行 `DELETE FROM clients WHERE USER_CODE=?`  

4. 私有工具方法  
   - `insertClient(...)`：拼装并执行 INSERT SQL，包含用户名、账号、密码、状态、时间戳、信令服务器地址等字段  
   - `deleteClientByUsercode(...)`：按 `USER_CODE` 删除记录  
   - `selectClientByUsercode(...)`：按 `USER_CODE` 查询并返回首行，调用 `mysql_store_result`/`mysql_fetch_row`  

5. 使用场景  
   - 注册流程中由业务逻辑调用 `UserRegister`，完成入库  
   - 登录验证时调用 `UserLogin`，检查返回行是否非空，若存在则允许登录  
   - 注销或用户销毁时调用 `UserDestroy`，从数据库删除该用户记录  

6. 可改进点  
   - 防止 SQL 注入：目前使用 `sprintf` 拼接 SQL，生产环境建议改用预编译语句（`mysql_stmt_prepare`）  
   - 连接断线重连、连接池：增强高可用性与性能  
   - 异常与错误处理：目前仅打印错误日志，可考虑抛出异常或返回错误码  

总之，ORMManager 封装了对 `clients` 表的所有基础操作，并通过单例保证整个登录服务器中只有一份数据库连接与统一的访问入口。


“ORM” 全称是 Object-Relational Mapping（对象–关系映射），本质上是把程序里的“对象”自动映射成数据库里的“表/行/列”。典型的 ORM 框架会：

- 根据 C++ 类/属性自动生成或解析 SQL  
- 在内存对象和数据库记录之间做自动转换，屏蔽 SQL 细节  

而我们这个 `ORMManager` 只是手写了几条 `INSERT`/`SELECT`/`DELETE` 的 SQL 语句，并没有：

- 定义对应的 C++ “实体类”（如 `User`）与数据库表结构的双向映射  
- 提供查询条件、字段筛选、分页等通用接口  
- 使用预编译语句或元编程生成 SQL  

它更像是一个“DAO”（Data Access Object）或“DBHelper”，只把常用的 CRUD 操作封装成了几个方法，但并不是真正的 ORM：  

- 名字用了 “ORM”，主要是想表达“管理数据库操作”的含义  
- 实际上没有自动映射功能，也没有把业务对象与表结构完全解耦  

如果要做真正的 ORM，可以考虑：  

1. 定义 C++ 实体类，利用元信息或宏／模板生成 CRUD 代码  
2. 使用预编译（`mysql_stmt_prepare`）+ 参数绑定来避免 SQL 拼接  
3. 提供更通用的查询接口（对象过滤、事务支持等）



TcpClient 是登录服务器监控模块中负责“心跳”上报的客户端组件，主要作用是定期采集本机内存使用率并通过 TCP 连接发送给负载均衡服务器（LoadBalanceServer）。它的设计要点和实现流程如下：

1. 构造与资源初始化  
   - 构造函数中将 `sockfd_=-1`、`file_=nullptr`、`isConnect_=false` 作为初始状态。  
   - 调用 `Create()`：  
     1) 打开 `/proc/meminfo` 文件，用于后续读取可用内存（`MemAvailable`）。  
     2) 调用 `sysinfo(&info_)` 获取总内存大小。  
     3) 创建 TCP socket（`socket(AF_INET, SOCK_STREAM, 0)`）。  
     4) 在 `Monitor_info_` 中预置负载均衡服务器 IP/port（默认 `192.168.31.30:9867`）。

2. 建立连接  
   - `bool Connect(std::string ip, uint16_t port)`：  
     • 填充 `sockaddr_in`，调用 `::connect()` 与指定服务器建立 TCP 连接。  
     • 成功则将 `isConnect_=true`，否则打印错误并返回 `false`。

3. 发送心跳  
   - 外部通过 `getMonitorInfo()` 触发一次心跳上报，内部调用 `get_mem_usage()`：  
     1) 从打开的 `/proc/meminfo` 跳过前两行，找到 “MemAvailable” 行并解析可用内存值（KB）。  
     2) 根据 `info_.totalram`（字节）计算总内存（KB），进而计算已用内存百分比：  
        `(total_kb - available_kb) / total_kb * 100`，并存到 `Monitor_info_.mem`。  
     3) 调用 `Send(uint8_t* data, uint32_t size)`，循环 `::send()` 直至完整发送 `Monitor_info_` 整个包。

4. 关闭与清理  
   - `Close()`：如果连接已建立，关闭文件指针、关闭 socket，并将 `isConnect_` 重置为 `false`。  
   - 析构函数 `~TcpClient()` 自动调用 `Close()`，确保资源释放。

5. 核心字段说明  
   - `FILE* file_`：指向 `/proc/meminfo` 打开的文件流，用于读取系统可用内存。  
   - `sysinfo info_`：保存总内存等系统信息，来自 `<sys/sysinfo.h>`。  
   - `int sockfd_`、`bool isConnect_`：管理 TCP socket 及其连接状态。  
   - `Monitor_body Monitor_info_`：协议定义的 “监控数据包” 结构，包含包头（长度/命令）、IP、port、mem 等字段。

6. 使用方式示例  
   ```cpp
   TcpClient client;
   client.Create();
   if (client.Connect("192.168.31.30", 9867)) {
       // 在定时器或主循环中周期调用：
       client.getMonitorInfo();
   }
   // 程序退出或不再上报时：
   client.Close();
   ```

通过以上流程，TcpClient 完成了“读取本机内存使用率 → 构造心跳包 → 通过 TCP 可靠发送” 的闭环，为负载均衡模块提供实时的节点负载数据。