# IM即时通讯服务器 - 完整开发文档

## 📑 目录

1. [系统架构](#1-系统架构)
2. [目录结构](#2-目录结构)
3. [核心流程图](#3-核心流程图)
4. [代码详解](#4-代码详解)
   - [4.1 主程序入口](#41-主程序入口)
   - [4.2 网络层](#42-网络层)
   - [4.3 业务处理层](#43-业务处理层)
   - [4.4 数据库层](#44-数据库层)
5. [通信协议](#5-通信协议)
6. [数据库设计](#6-数据库设计)
7. [安全机制](#7-安全机制)
8. [线程安全](#8-线程安全)

---

## 1. 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    IM服务器系统架构                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐        ┌──────────────────────┐      │
│  │  Windows     │  ←TCP→  │     Linux服务器      │      │
│  │  客户端      │        │                      │      │
│  │  (Qt)       │        │  ┌──────────────┐   │      │
│  └──────────────┘        │  │ Epoll事件循环 │   │      │
│                          │  └──────┬───────┘   │      │
│                          │         │           │      │
│                          │  ┌──────▼───────┐   │      │
│                          │  │ TcpServer    │   │      │
│                          │  │ (网络接收)    │   │      │
│                          │  └──────┬───────┘   │      │
│                          │         │           │      │
│                          │  ┌──────▼───────┐   │      │
│                          │  │ Mediator     │   │      │
│                          │  │ (调度器)     │   │      │
│                          │  └──────┬───────┘   │      │
│                          │         │           │      │
│                          │  ┌──────▼───────┐   │      │
│                          │  │ CKernel     │   │      │
│                          │  │ (业务处理)   │   │      │
│                          │  └──────┬───────┘   │      │
│                          │         │           │      │
│                          │  ┌──────▼───────┐   │      │
│                          │  │ CMySql       │   │      │
│                          │  │ (数据库)     │   │      │
│                          │  └──────────────┘   │      │
│                          └──────────────────────┘      │
│                                        │              │
│                                        ↓              │
│                          ┌────────────────────────┐    │
│                          │     MySQL数据库       │    │
│                          │  ┌─────────────────┐  │    │
│                          │  │  t_userinfo    │  │    │
│                          │  │  t_friend      │  │    │
│                          │  └─────────────────┘  │    │
│                          └────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 目录结构

```
im_server_linux/
├── main.cpp                    # 程序入口
├── IMServer.pro               # Qt项目文件
├── include/                   # 头文件目录
│   ├── net/
│   │   ├── def.h             # 协议定义和结构体
│   │   ├── inet.h           # 网络接口基类
│   │   ├── inetmediator.h   # 网络调度器接口
│   │   └── tcpserver.h      # TCP服务器头文件
│   ├── mediator/
│   │   └── tcpservermediator.h  # TCP调度器
│   ├── mysql/
│   │   └── cmysql.h         # MySQL数据库接口
│   └── kernel/
│       └── ckernel.h        # 核心业务处理类
└── source/                   # 源文件目录
    ├── net/
    │   └── tcpserver.cpp    # TCP服务器实现
    ├── mediator/
    │   └── tcpservermediator.cpp  # TCP调度器实现
    ├── mysql/
    │   └── cmysql.cpp       # MySQL数据库实现
    └── kernel/
        └── ckernel.cpp      # 核心业务处理实现
```

---

## 3. 核心流程图

### 3.1 服务器启动流程

```
main.cpp
   ↓
CKernel()
   ↓
startServer()
   ├─→ new TcpServerMediator()
   │        ↓
   │    TcpServer::initNet()
   │        ├─→ socket()
   │        ├─→ bind()
   │        ├─→ listen()
   │        └─→ epoll_create() + epoll_ctl()
   │
   ├─→ startRecv() [启动epoll线程]
   │        ↓
   │    epoll_wait() 循环
   │        ├─→ accept() [新连接]
   │        └─→ handleClientData() [有数据]
   │
   └─→ m_sql.ConnectMySql()
            ↓
        MySQL连接成功
```

### 3.2 客户端连接流程

```
客户端connect()
   ↓
服务器epoll_wait() [事件就绪]
   ↓
accept() 获取clientFd
   ↓
fcntl() 设置非阻塞
   ↓
epoll_ctl() 注册clientFd
   ↓
epoll_wait() 返回 [可读事件]
   ↓
recv() 接收数据
   ↓
handleClientData()
   ↓
CKernel::dealData()
   ↓
处理业务逻辑
```

### 3.3 注册流程

```
客户端发送注册请求
   ↓
服务器recv()
   ↓
CKernel::dealRegisterRq()
   ├─→ escapeString() 转义
   ├─→ 查询电话是否已存在
   ├─→ 查询用户名是否已存在
   ├─→ generateSalt() 生成盐值
   ├─→ hashPassword() 哈希密码
   ├─→ INSERT数据库
   └─→ 发送响应
```

### 3.4 登录流程

```
客户端发送登录请求
   ↓
服务器recv()
   ↓
CKernel::dealLoginRq()
   ├─→ escapeString() 转义
   ├─→ SELECT查询用户
   ├─→ 获取盐值和哈希密码
   ├─→ hashPassword() 哈希输入密码
   ├─→ 比对密码
   ├─→ 记录socket映射
   └─→ getUserInfoAddFriendInfo()
            ↓
        获取好友列表
            ↓
        通知所有好友上线
```

---

## 4. 代码详解

### 4.1 主程序入口

#### main.cpp

```cpp
#include "ckernel.h"
#include <iostream>
#include <thread>
#include <signal.h>
#include <atomic>

std::atomic<bool> g_running(true);  // 原子布尔，用于优雅退出

// 信号处理函数
void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_running = false;
        CKernel::pKernel->endServer();  // 清理资源
    }
}

int main() {
    std::cout << "IM Server starting..." << std::endl;

    // 注册信号处理（Ctrl+C优雅退出）
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    CKernel kernel;  // 创建核心处理类

    if (!kernel.startServer()) {
        std::cout << "启动服务器失败" << std::endl;
        return 1;
    }

    std::cout << "Server is running on port 67890..." << std::endl;

    // 主线程等待
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Server shutting down..." << std::endl;
    kernel.endServer();

    return 0;
}
```

**作用**：
- 程序入口点
- 初始化服务器
- 处理优雅退出信号
- 防止程序立即退出

---

### 4.2 网络层

#### 4.2.1 TcpServer - 初始化网络

**文件**: `tcpserver.cpp`

```cpp
bool TcpServer::initNet() {
    // 1. 创建TCP socket
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        std::cout << "sock error:" << errno << std::endl;
        return false;
    }
    std::cout << "sock success" << std::endl;

    // 2. 准备地址结构
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;                    // IPv4
    addr.sin_port = htons(_DEF_TCP_PORT);       // 端口67890
    addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // 监听所有网卡

    // 3. 设置地址复用（重启服务器时避免端口占用）
    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 4. 绑定地址
    if (bind(m_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "bind error:" << errno << std::endl;
        return false;
    }
    std::cout << "bind success" << std::endl;

    // 5. 开始监听
    if (listen(m_sock, _DEF_TCP_LISTEN_MAX) < 0) {
        std::cout << "listen error:" << errno << std::endl;
        return false;
    }
    std::cout << "listen success" << std::endl;

    // 6. 创建epoll实例
    m_epollFd = epoll_create(10000);  // 参数建议>0即可
    if (m_epollFd < 0) {
        std::cout << "epoll_create error:" << errno << std::endl;
        return false;
    }

    // 7. 注册监听socket到epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 监听读事件 + 边缘触发
    ev.data.fd = m_sock;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_sock, &ev) < 0) {
        std::cout << "epoll_ctl error:" << std::endl;
        return false;
    }

    std::cout << "Server is running on port 67890..." << std::endl;
    return true;
}
```

**作用**：
- 创建TCP socket
- 绑定端口和地址
- 创建epoll实例
- 注册监听socket

**关键参数**：
- `SO_REUSEADDR`: 允许地址复用，快速重启服务器
- `EPOLLIN`: 监听可读事件
- `EPOLLET`: 边缘触发模式，提高效率

---

#### 4.2.2 TcpServer - Epoll事件循环

```cpp
void TcpServer::recvData() {
    struct epoll_event events[10000];  // 事件数组
    int readynum;

    while (run) {
        // 阻塞等待事件，超时1秒
        readynum = epoll_wait(m_epollFd, events, 10000, 1000);

        for (int i = 0; i < readynum; ++i) {
            int fd = events[i].data.fd;

            if (fd == m_sock) {
                // ========== 监听socket可读 = 有新连接 ==========
                struct sockaddr_in clientAddr;
                socklen_t addrLen = sizeof(clientAddr);
                
                // 接受连接
                int clientFd = accept(m_sock, 
                                      (struct sockaddr*)&clientAddr, 
                                      &addrLen);

                if (clientFd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        std::cout << "accept error:" << errno << std::endl;
                    }
                    continue;
                }

                std::cout << "client connected from:" 
                          << inet_ntoa(clientAddr.sin_addr) 
                          << ":" << ntohs(clientAddr.sin_port) 
                          << " socket fd:" << clientFd << std::endl;

                // 设置非阻塞模式
                int flag = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flag | O_NONBLOCK);

                // 注册到epoll，监听客户端socket
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP;
                ev.data.fd = clientFd;
                
                if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
                    std::cout << "epoll_ctl add client error" << std::endl;
                    close(clientFd);
                }
                
            } else {
                // ========== 客户端socket有事件 ==========
                if (events[i].events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) {
                    handleClientData(fd);  // 处理数据
                }
            }
        }
    }
}
```

**Epoll事件类型**：
- `EPOLLIN`: 可读
- `EPOLLOUT`: 可写
- `EPOLLET`: 边缘触发
- `EPOLLRDHUP`: 对端关闭连接
- `EPOLLHUP`: 挂起

---

#### 4.2.3 TcpServer - 数据接收

```cpp
void TcpServer::handleClientData(int clientFd) {
    int packLen = 0;
    int nOffset = 0;
    int nRecvNum = 0;

    std::cout << "handleClientData called for fd:" << clientFd << std::endl;

    // 1. 先接收4字节的长度字段
    nRecvNum = recv(clientFd, (char*)&packLen, sizeof(int), 0);
    std::cout << "recv packLen: " << packLen << " bytes" << std::endl;

    if (nRecvNum > 0) {
        // 2. 验证数据包大小
        if (packLen <= 0 || packLen > 1024 * 1024) {
            std::cout << "invalid packLen:" << packLen << std::endl;
            return;
        }
        
        // 3. 分配内存
        char* packBuf = new char[packLen];
        memset(packBuf, 0, packLen);

        // 4. 循环接收完整数据
        int remaining = packLen;
        while (remaining > 0) {
            nRecvNum = recv(clientFd, packBuf + nOffset, remaining, 0);
            
            if (nRecvNum > 0) {
                nOffset += nRecvNum;
                remaining -= nRecvNum;
            } else if (nRecvNum == 0) {
                // 客户端关闭连接
                std::cout << "client closed connection:" << clientFd << std::endl;
                delete[] packBuf;
                removeClient(clientFd);
                return;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;  // 非阻塞，暂时无数据，继续等待
                }
                std::cout << "recv err:" << errno << std::endl;
                delete[] packBuf;
                removeClient(clientFd);
                return;
            }
        }

        std::cout << "Data received complete! packLen=" << packLen << std::endl;
        
        // 5. 转发给上层处理
        m_pMediator->transmateData(packBuf, nOffset, clientFd);
        
    } else if (nRecvNum == 0) {
        // 客户端主动关闭
        std::cout << "client disconnected:" << clientFd << std::endl;
        removeClient(clientFd);
    } else {
        // 接收错误
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "recv failed:" << errno << std::endl;
            removeClient(clientFd);
        }
    }
}
```

**数据接收流程**：
1. 接收4字节长度
2. 验证长度有效性
3. 分配内存
4. 循环接收直到完成
5. 转发给业务层

---

#### 4.2.4 TcpServer - 数据发送

```cpp
bool TcpServer::sendData(char* data, int len, unsigned long to) {
    if (nullptr == data || len <= 0) {
        std::cout << "data error" << std::endl;
        return false;
    }

    int clientFd = static_cast<int>(to);

    // 1. 先发送长度（4字节）
    int totalSent = 0;
    int remaining = sizeof(int);
    char* lenBuf = reinterpret_cast<char*>(&len);
    
    while (remaining > 0) {
        int nSendNum = send(clientFd, lenBuf + totalSent, remaining, 0);
        if (nSendNum < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  // 缓冲区满，继续尝试
            }
            std::cout << "len send error:" << errno << std::endl;
            return false;
        }
        totalSent += nSendNum;
        remaining -= nSendNum;
    }

    // 2. 再发送数据（循环发送直到完成）
    totalSent = 0;
    while (len > 0) {
        int nSendNum = send(clientFd, data + totalSent, len, 0);
        if (nSendNum < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            std::cout << "send error:" << errno << std::endl;
            return false;
        }
        totalSent += nSendNum;
        len -= nSendNum;
    }

    return true;
}
```

**关键点**：
- 先发送长度，再发送数据
- 使用循环确保完整发送
- 处理EAGAIN/EWOULDBLOCK

---

### 4.3 业务处理层

#### 4.3.1 CKernel - 初始化

**文件**: `ckernel.cpp`

```cpp
CKernel::CKernel() : m_pMediator(nullptr) {
    pKernel = this;  // 保存单例指针
    setProtocol();    // 设置协议处理函数
}

void CKernel::setProtocol() {
    std::cout << __func__ << std::endl;
    memset(m_protocol, 0, sizeof(m_protocol));
    
    // 初始化协议处理函数指针数组
    m_protocol[DEF_REGISTER_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRegisterRq;
    m_protocol[DEF_LOGIN_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealLoginRq;
    m_protocol[DEF_CHATMSG_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealChatRq;
    m_protocol[DEF_FRIEND_OFFLINE - DEF_PROTOCOL_BASE] = &CKernel::dealOfflineRq;
    m_protocol[DEF_ADD_FRIEND_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealAddFriRq;
    m_protocol[DEF_ADD_FRIEND_RS - DEF_PROTOCOL_BASE] = &CKernel::dealAddFriRs;
}
```

**作用**：
- 保存单例指针供其他模块使用
- 设置协议号到处理函数的映射

---

#### 4.3.2 CKernel - 启动服务器

```cpp
bool CKernel::startServer() {
    std::cout << __func__ << std::endl;
    
    // 1. 创建网络层
    m_pMediator = new TcpServerMediator;

    // 2. 打开网络
    if (!m_pMediator->openNet()) {
        std::cout << "打开网络失败" << std::endl;
        return false;
    }

    // 3. 启动epoll线程
    std::thread([this]() {
        m_pMediator->startRecv();
    }).detach();

    // 4. 连接数据库
    char ip[] = "127.0.0.1";
    char user[] = "root";
    char pass[] = "041206";
    char db[] = "20240919im";

    if (!m_sql.ConnectMySql(ip, user, pass, db, 3306)) {
        std::cout << "打开数据库失败" << std::endl;
        return false;
    }

    return true;
}
```

---

#### 4.3.3 CKernel - 消息分发

```cpp
void CKernel::dealData(char* data, int len, unsigned long from) {
    std::cout << __func__ << std::endl;
    
    // 1. 获取协议类型
    packageType type = *(packageType*)data;

    // 2. 计算数组索引
    int index = type - DEF_PROTOCOL_BASE;
    
    // 3. 检查索引有效性
    if (index >= 0 && index < DEF_PROTOCOL_COUNT) {
        // 4. 获取处理函数指针
        PFUN pf = m_protocol[index];
        if (pf) {
            // 5. 调用对应的处理函数
            (this->*pf)(data, len, from);
        } else {
            std::cout << "type2:" << type << std::endl;
        }
    } else {
        std::cout << "type1:" << type << std::endl;
    }

    // 6. 释放内存
    delete[] data;
}
```

**协议分发原理**：
```
协议号1000 ──┬──> m_protocol[0] ──→ dealRegisterRq
            │
协议号1002 ──┼──> m_protocol[2] ──→ dealLoginRq
            │
协议号1005 ──┴──> m_protocol[5] ──→ dealChatRq
```

---

#### 4.3.4 CKernel - 注册处理

```cpp
void CKernel::dealRegisterRq(char* data, int len, unsigned long from) {
    std::cout << "========== 注册请求 ==========" << std::endl;
    
    PROT_REGISTER_RQ* rq = (PROT_REGISTER_RQ*)data;
    
    std::cout << "用户名: " << rq->name << std::endl;
    std::cout << "电话: " << rq->tel << std::endl;
    std::cout << "密码: " << rq->pass << std::endl;

    // 1. 转义字符串（防止SQL注入）
    std::string safeName = escapeString(rq->name);
    std::string safeTel = escapeString(rq->tel);
    
    // 2. 检查输入长度
    if (safeName.length() == 0 || safeTel.length() == 0) {
        PROT_REGISTER_RS rs;
        rs.result = 3;  // 参数错误
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // 3. 检查电话是否已注册
    char checkSql[256];
    snprintf(checkSql, sizeof(checkSql), 
             "SELECT tel FROM t_userinfo WHERE tel = '%s'", safeTel.c_str());
    std::list<std::string> checkLst;
    
    if (m_sql.SelectMySql(checkSql, 1, checkLst) && checkLst.size() > 0) {
        PROT_REGISTER_RS rs;
        rs.result = DEF_REGISTER_TEL_EXIST;  // 电话已存在
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // 4. 检查用户名是否已注册
    snprintf(checkSql, sizeof(checkSql), 
             "SELECT tel FROM t_userinfo WHERE name = '%s'", safeName.c_str());
    checkLst.clear();
    if (m_sql.SelectMySql(checkSql, 1, checkLst) && checkLst.size() > 0) {
        PROT_REGISTER_RS rs;
        rs.result = DEF_REGISTER_NAME_EXIST;  // 用户名已存在
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // 5. 生成随机盐值
    std::string salt = generateSalt();
    std::cout << "生成盐值: " << salt << std::endl;
    
    // 6. 哈希密码
    std::string hashedPassword = hashPassword(std::string(rq->pass) + salt);
    std::cout << "哈希密码: " << hashedPassword.substr(0, 20) << "..." << std::endl;
    
    // 7. 插入数据库
    char sql[1024];
    snprintf(sql, sizeof(sql), 
             "INSERT INTO t_userinfo (name, tel, pass, salt, iconid, feeling) "
             "VALUES ('%s', '%s', '%s', '%s', 8, '在线')",
             safeName.c_str(), safeTel.c_str(), 
             hashedPassword.c_str(), salt.c_str());
    
    std::cout << "执行SQL: " << sql << std::endl;
    
    if (!m_sql.UpdateMySql(sql)) {
        std::cout << "插入数据库失败" << std::endl;
        return;
    }
    std::cout << "INSERT执行成功！" << std::endl;

    // 8. 发送成功响应
    PROT_REGISTER_RS rs;
    rs.result = DEF_REGISTER_SUC;
    m_pMediator->sendData((char*)&rs, sizeof(rs), from);
}
```

---

#### 4.3.5 CKernel - 登录处理

```cpp
void CKernel::dealLoginRq(char* data, int len, unsigned long from) {
    std::cout << "========== 登录请求 ==========" << std::endl;
    
    PROT_LOGIN_RQ* rq = (PROT_LOGIN_RQ*)data;
    std::cout << "电话: " << rq->tel << std::endl;
    std::cout << "密码: " << rq->pass << std::endl;

    std::string safeTel = escapeString(rq->tel);
    
    // 1. 查询用户
    char sql[1024];
    snprintf(sql, sizeof(sql), 
             "SELECT pass, salt, id FROM t_userinfo WHERE tel = '%s'", 
             safeTel.c_str());
    
    std::list<std::string> lstStr;
    std::cout << "执行查询SQL: " << sql << std::endl;
    
    if (!m_sql.SelectMySql(sql, 3, lstStr) || lstStr.size() < 3) {
        std::cout << "未找到用户" << std::endl;
        PROT_LOGIN_RS rs;
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }
    
    // 2. 获取数据库中的密码哈希和盐值
    std::string storedHash = lstStr.front();
    lstStr.pop_front();
    std::string salt = lstStr.front();
    lstStr.pop_front();
    std::string userIdStr = lstStr.front();
    lstStr.pop_front();
    
    std::cout << "找到用户: id=" << userIdStr << std::endl;
    
    // 3. 转换用户ID
    int userId = 0;
    try {
        userId = stoi(userIdStr);
    } catch (...) {
        std::cout << "转换用户ID失败" << std::endl;
        PROT_LOGIN_RS rs;
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }
    
    // 4. 哈希输入的密码
    std::string inputHash = hashPassword(std::string(rq->pass) + salt);
    std::cout << "输入密码哈希: " << inputHash.substr(0, 20) << "..." << std::endl;
    
    // 5. 比对密码
    if (inputHash == storedHash) {
        std::cout << "密码验证成功！" << std::endl;
        
        PROT_LOGIN_RS rs;
        rs.result = DEF_LOGIN_SUC;
        rs.userId = userId;
        
        // 6. 记录socket映射
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_mapIdToSocket[userId] = from;
            m_mapSocketToId[(int)from] = userId;
        }
        
        // 7. 发送响应
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        
        // 8. 获取好友列表并通知
        getUserInfoAddFriendInfo(userId);
        return;
    } else {
        std::cout << "密码错误！" << std::endl;
        PROT_LOGIN_RS rs;
        rs.result = DEF_LOGIN_PASSERR;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
    }
}
```

---

#### 4.3.6 CKernel - 获取好友信息

```cpp
void CKernel::getUserInfoAddFriendInfo(int userId) {
    std::cout << __func__ << std::endl;
    
    // 1. 获取自己的信息
    PROT_FRIEND_INFO userInfo;
    getInfoById(userId, (char*)&userInfo);

    // 2. 发送自己的信息给自己
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_mapIdToSocket.count(userId) > 0) {
            std::cout << "发送自己的信息到 socket=" 
                      << m_mapIdToSocket[userId] << std::endl;
            m_pMediator->sendData((char*)&userInfo, 
                                  sizeof(userInfo), 
                                  m_mapIdToSocket[userId]);
        }
    }

    // 3. 查询好友列表
    char friendSql[512];
    snprintf(friendSql, sizeof(friendSql), 
             "SELECT idB FROM t_friend WHERE idA = %d", userId);
    std::cout << "执行SQL: " << friendSql << std::endl;
    
    std::list<std::string> friendLst;
    if (m_sql.SelectMySql(friendSql, 1, friendLst)) {
        std::cout << "找到好友数量: " << friendLst.size() << std::endl;
        
        while (friendLst.size() > 0) {
            try {
                int friendId = stoi(friendLst.front());
                friendLst.pop_front();
                
                // 4. 获取好友信息
                PROT_FRIEND_INFO friendInfo;
                getInfoById(friendId, (char*)&friendInfo);

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    
                    // 5. 发送好友信息给自己
                    if (m_mapIdToSocket.count(userId) > 0) {
                        std::cout << "发送好友 " << friendId 
                                  << " 信息给用户 " << userId << std::endl;
                        m_pMediator->sendData((char*)&friendInfo, 
                                             sizeof(friendInfo), 
                                             m_mapIdToSocket[userId]);
                    }

                    // 6. 通知好友自己上线了
                    if (m_mapIdToSocket.count(friendId) > 0) {
                        std::cout << "通知好友 " << friendId 
                                  << " 用户 " << userId << " 上线" << std::endl;
                        m_pMediator->sendData((char*)&userInfo, 
                                             sizeof(userInfo), 
                                             m_mapIdToSocket[friendId]);
                    }
                }
            } catch (...) {
                continue;
            }
        }
        std::cout << "好友通知完成" << std::endl;
    }
}
```

---

#### 4.3.7 CKernel - 添加好友请求

```cpp
void CKernel::dealAddFriRq(char* data, int len, unsigned long from) {
    std::cout << "========== 添加好友请求 ==========" << std::endl;
    
    PROT_ADD_FRIEND_RQ* rq = (PROT_ADD_FRIEND_RQ*)data;
    
    std::cout << "我的ID: " << rq->userId << std::endl;
    std::cout << "我的名字: " << rq->usename << std::endl;
    std::cout << "要添加的好友名字: " << rq->friendName << std::endl;

    std::string safeFriendName = escapeString(rq->friendName);
    
    // 1. 检查输入
    if (safeFriendName.length() == 0 || 
        safeFriendName.length() > DEF_INFO_LEN - 1) {
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // 2. 查询好友是否存在
    char findFriendSql[256];
    snprintf(findFriendSql, sizeof(findFriendSql), 
             "SELECT id FROM t_userinfo WHERE name = '%s'", 
             safeFriendName.c_str());
    std::cout << "执行SQL: " << findFriendSql << std::endl;
    
    std::list<std::string> findFriendLst;
    
    if (!m_sql.SelectMySql(findFriendSql, 1, findFriendLst) || 
        findFriendLst.size() == 0) {
        std::cout << "好友不存在!" << std::endl;
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }
    
    // 3. 获取好友ID
    int friendId = 0;
    try {
        friendId = stoi(findFriendLst.front());
    } catch (...) {
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }
    
    std::cout << "好友ID: " << friendId << std::endl;
    
    // 4. 检查好友是否在线，转发请求
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "当前在线用户数: " << m_mapIdToSocket.size() << std::endl;
        
        if (m_mapIdToSocket.count(friendId) > 0) {
            std::cout << "好友在线! 转发请求到好友" << std::endl;
            m_pMediator->sendData(data, len, m_mapIdToSocket[friendId]);
        } else {
            std::cout << "好友不在线，发送离线响应" << std::endl;
            PROT_ADD_FRIEND_RS rs;
            rs.result = DEF_ADD_FRIEND_OFFLINE;
            strcpy(rs.friendName, rq->friendName);
            m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        }
    }
}
```

---

### 4.4 数据库层

#### 4.4.1 CMySql - 连接数据库

**文件**: `cmysql.cpp`

```cpp
bool CMySql::ConnectMySql(char *host, char *user, char *pass, 
                         char *db, short nport) {
    // 1. 建立连接
    if (!mysql_real_connect(m_sock, host, user, pass, db, nport, 
                           nullptr, CLIENT_MULTI_STATEMENTS)) {
        std::cout << "连接数据库失败:" << mysql_error(m_sock) << std::endl;
        return false;
    }
    
    // 2. 设置字符集
    mysql_set_character_set(m_sock, "utf8");
    mysql_query(m_sock, "SET NAMES utf8mb3");
    
    return true;
}
```

---

#### 4.4.2 CMySql - 查询数据

```cpp
bool CMySql::SelectMySql(const char* szSql, int nColumn, 
                        std::list<std::string>& lstStr) {
    // 1. 执行查询
    if (mysql_query(m_sock, szSql)) {
        std::cout << "查询数据库失败:" << mysql_error(m_sock) << std::endl;
        return false;
    }

    // 2. 获取结果集
    m_results = mysql_store_result(m_sock);
    if (nullptr == m_results) {
        std::cout << "查询结果为空" << std::endl;
        return false;
    }

    // 3. 遍历结果
    while ((m_record = mysql_fetch_row(m_results))) {
        for (int i = 0; i < nColumn; i++) {
            if (!m_record[i]) {
                lstStr.push_back("");  // NULL值
            } else {
                lstStr.push_back(m_record[i]);
            }
        }
    }
    
    // 4. 释放结果集
    mysql_free_result(m_results);
    return true;
}
```

---

#### 4.4.3 CMySql - 更新数据

```cpp
bool CMySql::UpdateMySql(const char* szSql) {
    if (!szSql) {
        return false;
    }
    
    if (mysql_query(m_sock, szSql)) {
        std::cout << "UpdateMySql failed: " << mysql_error(m_sock) << std::endl;
        std::cout << "SQL: " << szSql << std::endl;
        return false;
    }
    return true;
}
```

---

## 5. 通信协议

### 5.1 协议结构

```
┌─────────────────────────────────────┐
│         网络数据包格式              │
├─────────────────────────────────────┤
│  [4字节: 数据包长度]                 │
│  [N字节: 数据包内容]                │
└─────────────────────────────────────┘

数据包内容:
┌─────────────────────────────────────┐
│  [4字节: 协议类型]                  │
│  [其他: 协议数据]                   │
└─────────────────────────────────────┘
```

### 5.2 协议号定义

```cpp
#define DEF_PROTOCOL_BASE   1000

// 注册
#define DEF_REGISTER_RQ     (DEF_PROTOCOL_BASE+0)  // 1000
#define DEF_REGISTER_RS     (DEF_PROTOCOL_BASE+1)  // 1001

// 登录
#define DEF_LOGIN_RQ        (DEF_PROTOCOL_BASE+2)  // 1002
#define DEF_LOGIN_RS        (DEF_PROTOCOL_BASE+3)  // 1003

// 聊天
#define DEF_CHATMSG_RQ      (DEF_PROTOCOL_BASE+5)  // 1005
#define DEF_CHATMSG_RS      (DEF_PROTOCOL_BASE+6)  // 1006

// 添加好友
#define DEF_ADD_FRIEND_RQ   (DEF_PROTOCOL_BASE+7)  // 1007
#define DEF_ADD_FRIEND_RS   (DEF_PROTOCOL_BASE+8)  // 1008

// 下线
#define DEF_FRIEND_OFFLINE  (DEF_PROTOCOL_BASE+9)  // 1009

// 好友信息
#define DEF_FRIEND_INFO     (DEF_PROTOCOL_BASE+10) // 1010
```

### 5.3 协议详情

#### 注册请求 (DEF_REGISTER_RQ)
```cpp
struct PROT_REGISTER_RQ {
    packageType packtype;  // 1000
    char name[20];        // 用户名
    char tel[20];         // 电话
    char pass[20];        // 密码（明文）
};
```

#### 注册响应 (DEF_REGISTER_RS)
```cpp
struct PROT_REGISTER_RS {
    packageType packtype;  // 1001
    int result;            // 结果码
};
// result: 0=成功, 1=用户名已存在, 2=电话已存在
```

#### 登录请求 (DEF_LOGIN_RQ)
```cpp
struct PROT_LOGIN_RQ {
    packageType packtype;  // 1002
    char tel[20];         // 电话
    char pass[20];        // 密码（明文）
};
```

#### 登录响应 (DEF_LOGIN_RS)
```cpp
struct PROT_LOGIN_RS {
    packageType packtype;  // 1003
    int userId;            // 用户ID
    int result;            // 结果码
};
// result: 0=成功, 1=用户不存在, 2=密码错误
```

#### 添加好友请求 (DEF_ADD_FRIEND_RQ)
```cpp
struct PROT_ADD_FRIEND_RQ {
    packageType packtype;  // 1007
    int userId;            // 我的ID
    char usename[20];      // 我的用户名
    char friendName[20];  // 要添加的好友名字
};
```

#### 添加好友响应 (DEF_ADD_FRIEND_RS)
```cpp
struct PROT_ADD_FRIEND_RS {
    packageType packtype;  // 1008
    int result;            // 结果码
    int userId;            // 发送者ID
    int friendId;          // 接收者ID
    char userName[20];     // 发送者用户名
    char friendName[20];   // 接收者用户名
};
// result: 0=接受, 1=离线, 2=拒绝, 3=不存在
```

---

## 6. 数据库设计

### 6.1 用户信息表

```sql
CREATE TABLE t_userinfo (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(20) NOT NULL UNIQUE,
    tel VARCHAR(20) NOT NULL UNIQUE,
    pass VARCHAR(128) NOT NULL,        -- SHA256哈希
    salt VARCHAR(64) NOT NULL,         -- 随机盐值
    iconid INT DEFAULT 8,              -- 头像ID
    feeling VARCHAR(50) DEFAULT '在线', -- 心情
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
```

### 6.2 好友关系表

```sql
CREATE TABLE t_friend (
    idA INT NOT NULL,                  -- 用户A
    idB INT NOT NULL,                 -- 用户B（A的好友）
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (idA, idB),           -- 联合主键
    INDEX idx_idA (idA),
    INDEX idx_idB (idB)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
```

---

## 7. 安全机制

### 7.1 密码安全

#### 密码哈希流程
```
用户注册:
┌──────────┐    ┌──────────────┐    ┌─────────────────┐
│ 明文密码 │ + │ 随机盐值     │ →  │ SHA256哈希      │
│          │    │ (generateSalt) │   │                 │
└──────────┘    └──────────────┘    └─────────────────┘
                                            ↓
                                    存入数据库
```

#### 代码实现

```cpp
// 生成随机盐值
std::string CKernel::generateSalt() {
    std::random_device rd;          // 真随机数
    std::mt19937 gen(rd());        // 梅森旋转算法
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        int val = dis(gen);
        ss << std::hex 
           << std::setw(2) 
           << std::setfill('0') 
           << val;
    }
    return ss.str();  // 返回32字符的十六进制字符串
}

// SHA256哈希
std::string CKernel::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];  // 32字节
    
    SHA256((unsigned char*)(password).c_str(), 
           password.length(), 
           hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex 
           << std::setw(2) 
           << std::setfill('0') 
           << (int)hash[i];
    }
    return ss.str();  // 返回64字符的十六进制字符串
}
```

### 7.2 SQL注入防护

#### 转义函数
```cpp
std::string CKernel::escapeString(const std::string& input) {
    std::string result;
    result.reserve(input.length() * 2);
    
    for (char c : input) {
        switch (c) {
            case '\'': result += "\\'"; break;   // 单引号
            case '\"': result += "\\\""; break;   // 双引号
            case '\\': result += "\\\\"; break;   // 反斜杠
            case '\n': result += "\\n"; break;    // 换行
            case '\r': result += "\\r"; break;    // 回车
            case '\t': result += "\\t"; break;     // Tab
            case '\0': result += "\\0"; break;     // 空字符
            case '\x1a': result += "\\Z"; break;   // Ctrl+Z
            default: result += c; break;
        }
    }
    
    return result;
}
```

---

## 8. 线程安全

### 8.1 互斥锁保护

```cpp
class CKernel {
private:
    std::mutex m_mutex;  // 互斥锁
    std::map<int, int> m_mapIdToSocket;      // 用户ID → socket
    std::map<int, int> m_mapSocketToId;      // socket → 用户ID
};

// 使用示例
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapIdToSocket[userId] = from;
    m_mapSocketToId[(int)from] = userId;
}  // 作用域结束时自动释放锁
```

### 8.2 为什么需要互斥锁

```
线程A (处理登录)              线程B (处理好友请求)
    │                            │
    ├─ read m_mapIdToSocket      │
    │                            ├─ write m_mapIdToSocket
    │                            │    └─ 修改中...
    ├─ write m_mapIdToSocket     │
    └─ 读取到不完整的数据        │
                                ├─ read m_mapIdToSocket
                                └─ 读取到不完整数据

加锁后:
线程A (处理登录)              线程B (处理好友请求)
    │                            │
    ├─ lock()                    │
    │  └─ 等待...                │
    │                            ├─ lock()
    │                            │  └─ 等待...
    ├─ write m_mapIdToSocket     │
    │  └─ 完成                   │
    ├─ unlock()                  │
    │                            ├─ lock()
    │                            │  └─ 获取锁
    │                            ├─ read m_mapIdToSocket
    │                            │  └─ 读取完整数据
    │                            ├─ unlock()
```

---

## 📝 附录

### A. 编译和运行

```bash
# 编译
cd im_server_linux
qmake IMServer.pro
make

# 运行
./bin/im_server
```

### B. 数据库初始化

```sql
CREATE DATABASE IF NOT EXISTS 20240919im CHARACTER SET utf8mb3;
USE 20240919im;

CREATE TABLE IF NOT EXISTS t_userinfo (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(20) NOT NULL UNIQUE,
    tel VARCHAR(20) NOT NULL UNIQUE,
    pass VARCHAR(128) NOT NULL,
    salt VARCHAR(64) NOT NULL DEFAULT '',
    iconid INT DEFAULT 8,
    feeling VARCHAR(50) DEFAULT '在线',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;

CREATE TABLE IF NOT EXISTS t_friend (
    idA INT NOT NULL,
    idB INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (idA, idB),
    INDEX idx_idA (idA),
    INDEX idx_idB (idB)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
```

### C. 常见问题

1. **端口被占用**: `pkill -9 im_server`
2. **数据库连接失败**: 检查MySQL服务状态
3. **字符集错误**: 确保使用utf8mb3

---

## ✅ 文档结束

本文档详细解释了IM服务器的架构、代码实现、安全机制和线程安全。

**祝你开发愉快！** 🚀
