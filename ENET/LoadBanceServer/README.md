负载均衡服务器中的“负载”指的是LoginServer里的mem变量（见LoginServer中的TcpClient）
变量 `mem` 存储的是当前系统的内存使用率（已使用内存占总内存的百分比），它的变化过程和依据大致如下：

1. 调用 `get_mem_usage()` 时，先通过 `sysinfo(&info_)` 在 `Create()` 中拿到总内存信息，存储于 `info_.totalram`（字节数）。  
2. 打开并读取 `/proc/meminfo`：  
   - 跳过前两行，找到标记为 `"MemAvailable"` 的行，解析出可用内存 `avimem`（单位是 KB）。  
3. 计算：  
   - 总内存（KB） = `info_.totalram / 1024.0`  
   - 已用内存（KB） = 总内存 – `avimem`  
   - 使用率 (%) = (已用内存 / 总内存) × 100  
4. 最后把浮点计算结果强转成 `uint8_t` 赋给 `Monitor_info_.mem`，随系统可用内存实时更新。  
   
所以，`mem` 会随着 `/proc/meminfo` 中 `MemAvailable` 值的变化而变化，反映当前内存使用的百分比。

# 负载均衡指的是每次选择内存使用率最小的服务器（即mem最小），所以会涉及排序算法，每次查找内存使用率最小的服务节点，向客户端返回服务节点的ip地址和端口。

负载均衡服务器（LoadBanceServer）利用各后端服务上报的“内存使用率”作为负载度量指标，通过以下流程实现最小负载调度：  

1. 后端服务心跳上报  
   - 每个后端服务通过 `LoadBanceConnection` 连接，不断发送包含当前 `Monitor_body`（结构中含有 `mem` 字段——内存使用率百分比）的心跳包。  
   - `LoadBanceConnection::HandleMinoterInfo()` 接收到心跳后调用：  
     ```cpp
     server->UpdateMonitor(socket_, body);
     ```  
   - `LoadBanceServer::UpdateMonitor(fd, info)` 在互斥锁保护下将最新的 `Monitor_body*` 存入 `monitorInfos_[fd]`。

2. 维护负载表  
   - `monitorInfos_` 是一个 `std::map<int, Monitor_body*>`，key 为后端连接的文件描述符，value 为最新心跳信息。  
   - 更新与查询都在 `std::mutex` 保护下完成，保证并发安全。

3. 查询最优节点  
   - 当有新的客户端登录请求（`Login` 命令）时，`LoadBanceConnection::HnadleLogin()` 会调用：  
     ```cpp
     Monitor_body* best = server->GetMonitorInfo();
     reply.ip   = best->ip;//回复最优ip地址
     reply.port = best->port;//回复最优端口
     ```  
   - `LoadBanceServer::GetMonitorInfo()` 实现：  
     1) 将 `monitorInfos_` 中的 `(fd, Monitor_body*)` 拷贝到 `std::vector<MinotorPair>`；  
     2) 用自定义比较器 `CmpByValue`（按 `mem` 升序）对 vector 排序；  
     3) 返回排序后第一个元素，即当前内存使用率mem最小的服务节点（获取ip地址和端口）。

4. 客户端接入  
   - 客户端收到 `LoginReply` 包后，即可直接连接并使用该最优后端服务的 `ip:port`，完成业务请求。  

5. 算法特点  
   - **动态实时**：后端通过心跳不断上报最新负载；  
   - **简单高效**：排序开销可接受（后端节点数通常不大），也可改用优先队列／小顶堆优化；  
   - **可扩展**：若将来要加入多维度指标（CPU、带宽等），只需在 `Monitor_body` 中扩展字段和比较器即可。

通过这套“心跳→更新→排序→调度”流程，LoadBanceServer 能够始终将新到的客户端请求分配给当前集群中最空闲、最稳定的后端节点，从而实现负载均衡。

下面对 LoadBanceConnection 类做一个详尽的分步解析，说明它在负载均衡服务器中的职责、核心成员及工作流程。

---

## 一、类的定位与继承关系

- LoadBanceConnection 继承自 `TcpConnection`  
  · 复用了基类对底层 socket 的读写、缓冲管理、事件回调等功能  
  · 额外添加了“登录分配”、“心跳上报”和“超时检测”等业务逻辑  

- 与 LoadBanceServer 协同工作  
  · 在登录时调用 `LoadBanceServer::GetMonitorInfo()` 分配可用监控节点  
  · 在心跳时调用 `LoadBanceServer::UpdateMonitor()` 更新节点状态  

---

## 二、成员变量

```cpp
int socket_;  
std::weak_ptr<LoadBanceServer> loadbance_server_;
```

- `socket_`：底层 TCP 描述符，用于在 `HandleMinoterInfo` 里上报心跳时标识是哪条连接  
- `loadbance_server_`：弱引用到服务器实例，避免循环引用；需要使用前通过 `.lock()` 检查是否还存活  

---

## 三、核心逻辑与流程

1. 构造函数：  
   - 调用基类 `TcpConnection(task_scheduler, sockfd)` 完成基本初始化  
   - 通过 `SetReadCallback` 绑定 `OnRead`，任何入站数据都会进入此回调  
   - 通过 `SetDisConnectCallback` 绑定 `DisConnection`，连接断开时清理或通知服务器  

2. OnRead(BufferReader& buffer)：  
   - 检测是否有可读数据  
   - 委托给 `HandleMessage(buffer)` 进一步解析和分发  

3. HandleMessage(BufferReader& buffer)：  
   - 先检查缓冲区是否至少包含一帧包头 `packet_head`  
   - 读取 `head->len` 判断整包是否就绪  
   - 根据 `head->cmd` 分发：  
     · `Login` → `HnadleLogin(buffer)`  
     · `Minotor` → `HandleMinoterInfo(buffer)`  
   - 最后调用 `buffer.Retrieve(head->len)` 丢弃已处理数据  

4. HnadleLogin(BufferReader& buffer)：  
   - 从缓冲区直接 reinterpret 为 `Login_Info*`，读取客户端发送的 timestamp  
   - 调用 `IsTimeout(info->timestamp)` 判断超时：  
     · 使用 `std::chrono::system_clock` 计算当前秒级时间戳  
     · 如果超时（差值 > 60 秒），回复 `cmd = ERROR`  
     · 否则通过 `loadbance_server_.lock()->GetMonitorInfo()` 获取监控节点 IP/端口  
   - 构造 `LoginReply` 并通过 `Send()` 一次性发送给客户端  

5. HandleMinoterInfo(BufferReader& buffer)：  
   - 把缓冲区数据 reinterpret 为 `Monitor_body*`  
   - 通过 `loadbance_server_.lock()->UpdateMonitor(socket_, body)` 上报最新状态  

6. IsTimeout(uint64_t timestamp)：  
   - 定义宏 `TIMEOUT = 60`（秒）  
   - 比较客户端发送的时间戳与当前系统时间戳之差，若超过 `TIMEOUT` 则视为超时  

7. DisConnection()：  
   - 客户端断开时的清理入口，可以在此通知服务器回收资源  

---

## 四、协同作用

- **负载均衡逻辑**：  
  客户端首次连接后发送登录包 → `HnadleLogin` 分配最优监控节点 → 客户端与该节点建立后续通信  
- **节点健康监测**：  
  各监控节点定时发送心跳包 → `HandleMinoterInfo` 更新状态 → 服务器可据此剔除失联节点  
- **超时保护**：  
  防止客户端长时间不发心跳而占用资源  

---

## 五、小结

LoadBanceConnection 将 TCP 连接层与负载均衡业务逻辑结合起来，通过对基类 `TcpConnection` 的回调机制重载，实现了：

- 一次性注册读／断开回调  
- 报文接收→包头完整性校验→命令分发  
- 超时检测、登录分配、心跳上报等功能  

它是负载均衡服务器中负责单个客户端或监控节点生命周期管理的重要组成。


LoadBanceServer.h 是“负载均衡”模块的入口和核心，主要职责为：

1. TCP 服务端管理  
   - 继承自 EdoyunNet::TcpServer，负责监听外部（LoginServer 或其它负载均衡客户端）的 TCP 连接。  
   - 重载 OnConnect，当有新连接进来时，为它创建一个 LoadBanceConnection 会话实例。

2. 工厂方法与生命周期  
   - static Create(EventLoop*)：工厂函数，返回 shared_ptr，强制使用堆分配并保证 enable_shared_from_this 能正常工作。  
   - 构造函数（private）：初始化 TcpServer 父类和内部 EventLoop 指针。  
   - 析构函数：遍历并 delete 掉 monitorInfos_ 中所有 Monitor_body*，避免内存泄漏。

3. 监控数据维护（monitorInfos_）  
   - 存储格式：std::map<int /*fd*/, Monitor_body*>  
     • key：后端服务连接的文件描述符  
     • value：对应该后端当前监控数据（CPU、内存等统计）  
   - 线程安全：所有 UpdateMonitor/GetMonitorInfo 操作都在 mutex_ 保护下执行。

4. UpdateMonitor(fd, info)  
   - 由 LoadBanceConnection 在接收到后端服务上报的监控数据包时调用。  
   - 将或更新 map 中对应 fd 的 Monitor_body* 指针。

5. GetMonitorInfo() → Monitor_body*  
   - 拷贝 map 内容到 vector<MinotorPair>，使用 CmpByValue（按 mem 成员升序）排序。  
   - 返回排序后第一个元素的 Monitor_body*，即当前内存使用最少、负载最轻的后端服务信息，用于后续分配请求。

6. OnConnect(socket)  
   - 当有新的客户端（通常是 LoginServer 或心跳连接）连接时，创建 LoadBanceConnection，将自身 shared_ptr 传入，便于 Connection 回调时上报监控数据到此服务器。

总体而言，LoadBanceServer 将多路后端服务的运行状态集中管理，通过 UpdateMonitor 汇总、GetMonitorInfo 决策最优节点，并在 TCP 层与各端保持会话，为上层提供“按最低负载分配后端服务”的能力。