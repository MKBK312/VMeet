# 多线程Reactor模型 - 完整说明

## 📋 概述

本IM服务器已成功改造为**多线程Reactor模型**，参考了你提供的C语言线程池代码架构。

---

## 🏗️ 架构对比

### 单线程Reactor模型（旧版本）
```
┌─────────────────────────────────┐
│      主线程 (唯一)               │
│                                 │
│  epoll_wait()                  │
│      ↓                          │
│  handleClientData()           │
│      ↓                          │
│  CKernel.dealData()           │
│      ↓                          │
│  数据库查询（阻塞）             │
│      ↓                          │
│  sendData()                    │
└─────────────────────────────────┘
```

### 多线程Reactor模型（新版本）
```
┌────────────────────────────────────────────────────────┐
│                  主线程 (Reactor)                     │
│                                                         │
│  epoll_wait()                                          │
│      ↓                                                  │
│  事件就绪 → producer_add_task()                        │
│      ↓                                                  │
│  任务队列 ─────────────────────────────────────────────┐│
└─────────────────────────────────────────────────────│────┘
                                                         │
┌──────────┬──────────┬──────────┬──────────┐           │
│  工作线程1│  工作线程2│  工作线程3│  ...    │           │
│     ↓      │     ↓      │     ↓      │           │
│cusumer    │cusumer    │cusumer    │           │
│  处理任务  │  处理任务  │  处理任务  │           │
│     ↓      │     ↓      │     ↓      │           │
│数据库查询  │数据库查询  │数据库查询  │           │
└──────────┴──────────┴──────────┴──────────┘           │
                                                         │
                                                 线程池
```

---

## 📁 新增文件

### 1. 线程池封装类
- **头文件**: `include/net/threadpool.h`
- **实现**: `source/net/threadpool.cpp`

**特性**：
```cpp
class ThreadPool {
    int m_minThreads;      // 最小线程数 (默认10)
    int m_maxThreads;      // 最大线程数 (默认100)
    int m_queueMax;        // 队列最大长度 (默认1000)
    
    std::queue<std::function<void()>> m_taskQueue;  // 任务队列
    std::vector<pthread_t> m_workerThreads;          // 工作线程数组
    pthread_t m_managerThread;                         // 管理者线程
    
    // 自动扩缩容
    void managerThreadFunc();  // 监控线程
};
```

### 2. 任务处理器
- **头文件**: `include/net/task_handler.h`
- **实现**: `source/net/task_handler.cpp`

**任务类型**：
```cpp
TaskHandler::handleNewConnection()  // 处理新连接
TaskHandler::handleClientData()      // 处理客户端数据
```

---

## 🔄 工作流程

### 1. 初始化
```cpp
TcpServer::initNet()
    ↓
创建 epoll 实例
    ↓
创建 ThreadPool (10个工作线程)
    ↓
启动监听
```

### 2. 事件循环
```cpp
TcpServer::recvData()  // 主线程
    ↓
while (run) {
    epoll_wait()  // 等待事件
        ↓
    if (监听socket可读) {
        // 添加任务：处理新连接
        m_threadPool->addTask([]() {
            TaskHandler::handleNewConnection(&listenFd);
        });
    } else {
        // 添加任务：处理客户端数据
        m_threadPool->addTask([]() {
            TaskHandler::handleClientData(&clientFd);
        });
    }
}
```

### 3. 任务执行
```cpp
线程池中的工作线程
    ↓
从任务队列取任务
    ↓
执行任务函数
    ↓
调用 CKernel.dealData()
    ↓
数据库查询（线程池线程）
    ↓
发送响应
```

---

## ⚙️ 线程池配置

```cpp
// TcpServer::initNet()
m_threadPool = new ThreadPool(
    10,     // minThreads:  最小线程数
    100,    // maxThreads: 最大线程数
    1000    // queueMax:   任务队列最大长度
);
```

### 自动扩缩容机制

**扩容条件**：
- 任务队列长度 > 忙碌线程数 × 2
- 当前线程数 < 最大线程数
```cpp
if (taskSize > busy && alive < maxThreads) {
    // 创建新线程
}
```

**缩容条件**：
- 忙碌线程数 × 2 < 存活线程数
- 存活线程数 > 最小线程数
```cpp
if (busy * 2 < alive && alive > minThreads) {
    // 通知线程退出
}
```

---

## 📊 日志输出示例

### 启动日志
```
sock success
bind success
listen success
ThreadPool created: min=10, max=100, queue=1000
Server is running on port 6789...
```

### 管理者线程日志
```
[Manager] alive=10, busy=3, queue=50
[Manager] Created new thread, alive=11
```

### 工作线程处理日志
```
[ThreadPool] Worker 0x7f8d3c000b40 handling task...
[ThreadPool] Task completed
```

### 主线程日志
```
epoll_wait returned 5 events
Added task to thread pool
Added task to thread pool
Added task to thread pool
```

---

## 🔐 线程安全

### 1. 任务队列
```cpp
std::mutex m_queueMutex;
std::condition_variable m_taskCV;
std::condition_variable m_fullCV;
std::condition_variable m_emptyCV;

// 添加任务
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    while (queue.size() >= maxSize) {
        m_fullCV.wait(lock);  // 队列满，等待
    }
    queue.push(task);
    if (queue.size() == 1) {
        m_taskCV.notify_one();  // 唤醒工作线程
    }
}
```

### 2. socket操作
- **主线程**: epoll管理、socket注册
- **工作线程**: recv/send数据

**注意**: recv/send是线程安全的，但需要注意：
- socket必须在主线程添加到epoll
- 工作线程不修改socket的epoll状态

---

## 📈 性能提升

### 对比测试

| 指标 | 单线程 | 多线程(10线程) |
|------|--------|---------------|
| 并发处理能力 | 1个请求/次 | 10个请求/次 |
| CPU利用率 | 低（等待IO） | 更高（并行处理）|
| 响应时间 | 阻塞 | 非阻塞 |
| 数据库查询 | 串行 | 并行 |

### 典型场景

**场景1: 10个客户端同时发送消息**
```
单线程: 串行处理，总时间 = T1 + T2 + ... + T10
多线程: 并行处理，总时间 ≈ max(T1, T2, ..., T10)
```

**场景2: 数据库查询耗时100ms**
```
单线程: 客户端等待100ms + 处理时间
多线程: 其他客户端不被阻塞
```

---

## 🛠️ 使用方法

### 1. 编译
```bash
cd /home/mkbk/colin/im_server_linux
make clean && qmake IMServer.pro && make
```

### 2. 运行
```bash
./bin/im_server
```

### 3. 客户端连接
```
服务器IP: 127.0.0.1 (或实际IP)
端口: 6789
```

---

## 📝 注意事项

### 1. 端口配置
```cpp
#define _DEF_TCP_PORT (6789)  // 避免端口号溢出
```

### 2. 线程数配置
```cpp
// 根据服务器性能调整
int minThreads = 10;   // CPU核心数 × 2
int maxThreads = 100;  // 最大并发数预估
int queueMax = 1000;   // 任务队列长度
```

### 3. 内存管理
- 任务参数使用 `new` 分配
- 任务完成后在Lambda中 `delete`
```cpp
int* arg = new int(fd);
m_threadPool->addTask([arg]() {
    TaskHandler::handleClientData(arg);
    delete arg;  // 内存释放
});
```

---

## 🔍 调试建议

### 1. 开启详细日志
```cpp
// task_handler.cpp
std::cout << "[ThreadPool] Task started" << std::endl;
std::cout << "[ThreadPool] Task completed" << std::endl;
```

### 2. 监控线程池状态
```cpp
std::cout << "Busy threads: " << m_threadPool->getBusyThreadCount() << std::endl;
std::cout << "Alive threads: " << m_threadPool->getAliveThreadCount() << std::endl;
```

### 3. 检查任务队列
```cpp
std::cout << "Queue size: " << m_taskQueue.size() << std::endl;
```

---

## 🎯 适用场景

### ✅ 适合
- IO密集型应用
- 高并发短连接
- 数据库查询较多
- 需要充分利用多核CPU

### ❌ 不适合
- CPU密集型计算（视频编解码）
- 大量长时间阻塞操作
- 需要严格的事务一致性

---

## 📚 相关资料

- C++11 多线程编程
- std::thread, std::mutex, std::condition_variable
- POSIX threads (pthread)
- Epoll ET模式
- 生产者-消费者模式

---

## ✅ 总结

改造后的IM服务器成功实现了**多线程Reactor模型**，具有以下特点：

1. **高效并发**: 线程池处理业务逻辑
2. **自动扩缩容**: 根据负载动态调整线程数
3. **线程安全**: 完善的任务队列和同步机制
4. **性能提升**: 充分利用多核CPU
5. **代码清晰**: C++风格封装，易于维护

**现在你的服务器可以高效处理大量并发连接了！** 🚀
