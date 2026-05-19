# IM即时通讯客户端 - 完整开发文档

## 📑 目录

1. [系统架构](#1-系统架构)
2. [目录结构](#2-目录结构)
3. [核心流程图](#3-核心流程图)
4. [代码详解](#4-代码详解)
   - [4.1 主程序入口](#41-主程序入口)
   - [4.2 核心控制器 CKernel](#42-核心控制器-ckernel)
   - [4.3 登录对话框 LoginDialog](#43-登录对话框-logindialog)
   - [4.4 好友列表 FriendList](#44-好友列表-friendlist)
   - [4.5 好友组件 FriendItem](#45-好友组件-frienditem)
   - [4.6 聊天对话框 ChatDialog](#46-聊天对话框-chatdialog)
   - [4.7 网络层 Net](#47-网络层-net)
   - [4.8 中介层 Mediator](#48-中介层-mediator)
5. [通信协议](#5-通信协议)
6. [信号与槽机制](#6-信号与槽机制)
7. [编码转换](#7-编码转换)

---

## 1. 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                  IM客户端系统架构                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │              Windows 客户端 (Qt 5.12)             │   │
│  │                                                  │   │
│  │  ┌──────────┐    ┌──────────────┐               │   │
│  │  │ UI 层    │    │ LoginDialog  │               │   │
│  │  │          │    │ friendList   │               │   │
│  │  │          │    │ frienditem   │               │   │
│  │  │          │    │ ChatDialog   │               │   │
│  │  └────┬─────┘    └──────┬───────┘               │   │
│  │       │                 │                        │   │
│  │       │          signals/slots                   │   │
│  │       │                 │                        │   │
│  │  ┌────▼─────────────────▼──────┐               │   │
│  │  │         CKernel             │               │   │
│  │  │   (核心控制器 + 协议分发)    │               │   │
│  │  └────────────┬────────────────┘               │   │
│  │               │                                 │   │
│  │  ┌────────────▼────────────────┐               │   │
│  │  │    TcpClientMediator        │               │   │
│  │  │    (网络中介层)             │               │   │
│  │  └────────────┬────────────────┘               │   │
│  │               │                                 │   │
│  │  ┌────────────▼────────────────┐               │   │
│  │  │       TCPClient             │               │   │
│  │  │    (Winsock2 原始Socket)    │               │   │
│  │  └────────────┬────────────────┘               │   │
│  └───────────────┼────────────────────────────────┘   │
│                  │ TCP                                 │
│                  ▼                                     │
│     ┌────────────────────────┐                        │
│     │   Linux IM 服务器       │                        │
│     │   192.168.134.128:67890 │                        │
│     └────────────────────────┘                        │
└─────────────────────────────────────────────────────────┘
```

**三层架构：**

| 层 | 类 | 职责 |
|----|----|------|
| UI 层 | LoginDialog, friendList, frienditem, ChatDialog | 用户界面，数据展示 |
| 控制层 | CKernel | 协议分发，业务逻辑，信号槽绑定 |
| 网络层 | TcpClientMediator → TCPClient | TCP 连接，数据收发 |

---

## 2. 目录结构

```
IMClient/
├── main.cpp                       # 程序入口
├── IMClient.pro                   # Qt 项目文件
├── res.qrc                        # 资源文件（头像 + 图标）
├── ckernel.h                      # 核心控制器头文件
├── ckernel.cpp                    # 核心控制器实现（14KB，核心文件）
├── logindialog.h                  # 登录对话框头文件
├── logindialog.cpp                # 登录对话框实现
├── logindialog.ui                 # 登录对话框 UI 布局
├── friendlist.h                   # 好友列表窗口头文件
├── friendlist.cpp                 # 好友列表窗口实现
├── friendlist.ui                  # 好友列表 UI 布局
├── frienditem.h                   # 单个好友组件头文件
├── frienditem.cpp                 # 单个好友组件实现
├── frienditem.ui                  # 单个好友组件 UI 布局
├── chatdialog.h                   # 聊天对话框头文件
├── chatdialog.cpp                 # 聊天对话框实现
├── chatdialog.ui                  # 聊天对话框 UI 布局
├── net/                           # 网络传输层
│   ├── def.h                     # 协议定义（结构体 + 常量）
│   ├── TCPClient.h               # TCP 客户端头文件
│   ├── TCPClient.cpp             # TCP 客户端实现（Winsock2）
│   ├── TCP.h                     # TCP 服务端头文件（预留）
│   ├── TCP.cpp                   # TCP 服务端实现（预留）
│   ├── UDP.h                     # UDP 头文件（预留）
│   └── UDP.cpp                   # UDP 实现（预留）
├── Mediator/                      # 中介层
│   ├── INetMediator.h            # 中介抽象接口
│   ├── TcpClientMediator.h       # TCP 客户端中介头文件
│   ├── TcpClientMediator.cpp     # TCP 客户端中介实现
│   ├── TcpServerMediator.h       # TCP 服务端中介头文件（预留）
│   ├── UdpNetMediator.h          # UDP 中介头文件（预留）
│   └── UdpNetMediator.cpp        # UDP 中介实现（预留）
├── images/                        # UI 图标资源（27 个）
│   ├── login.png                 # 登录背景图
│   ├── send.png, clear.png ...   # 工具栏图标
└── tx/                            # 用户头像资源（36 个）
    ├── 0.png ~ 35.png            # 可选头像
```

---

## 3. 核心流程图

### 3.1 客户端启动流程

```
main.cpp
   ↓
QApplication 创建
   ↓
CKernel 构造（ckernel.cpp:6-48）
   ├─→ setProtocol()             ← 注册 8 个协议处理函数
   ├─→ new friendList            ← 创建好友列表窗口（隐藏）
   ├─→ new LoginDialog           ← 创建登录窗口
   ├─→ m_pLoginDlg->show()       ← 显示登录窗口
   ├─→ connect() × 6             ← 绑定信号槽
   ├─→ new TcpClientMediator     ← 创建网络中介
   ├─→ connect SIG_dealData      ← 绑定数据接收信号
   └─→ openNet()                 ← 连接服务器 192.168.134.128:67890
         ├─ TCPClient::initNet()
         │    ├─ WSAStartup()       加载 Winsock2
         │    ├─ socket()           创建 TCP socket
         │    ├─ connect()          连接服务器
         │    └─ _beginthreadex()   启动接收线程
         ├─ 成功 → 等待用户操作
         └─ 失败 → QMessageBox "打开网络失败" → exit(1)
```

### 3.2 注册流程

```
┌─────────────────────────────────────────────────────────────────┐
│  客户端                                                         │
│                                                                 │
│  用户点击"注册"按钮                                              │
│       ↓                                                         │
│  LoginDialog::on_pb_register_clicked()                          │
│       ├─ 读取输入：name, tel, pass                               │
│       ├─ 校验：非空、长度合法（name≤20, tel=11, pass≤20）        │
│       └─ emit sig_registerData(name, tel, pass)                 │
│              ↓                                                   │
│  CKernel::slots_registerData()                                  │
│       ├─ 打包 PROT_REGISTER_RQ { packtype=1000, name, tel, pass}│
│       └─ m_pMediator->sendData() 发送                           │
│              ↓                                                   │
│  TCPClient::sendData()                                          │
│       ├─ send(4字节长度=64)                                     │
│       └─ send(64字节结构体)                                     │
│                                                                 │
│  ──────────────────── TCP ──────────────────────────────→       │
│                                                                 │
│  服务端处理                                                      │
│       ├─ escapeString() 防注入                                  │
│       ├─ SELECT 检查电话/用户名是否已存在                         │
│       ├─ generateSalt() + hashPassword()                        │
│       └─ INSERT INTO t_userinfo                                 │
│                                                                 │
│  ←──────────────────── TCP ──────────────────────────────       │
│                                                                 │
│  TCPClient 接收线程收到数据                                       │
│       ↓                                                         │
│  TcpClientMediator::transmateData() → emit sig_dealData()       │
│       ↓                                                         │
│  CKernel::slot_delData()                                        │
│       ├─ type = *(packageType*)data  → 1001                     │
│       ├─ index = 1001 - 1000 = 1                                │
│       └─ m_protocol[1](data) → dealRegisterRs()                 │
│              ↓                                                   │
│  CKernel::dealRegisterRs()                                      │
│       ├─ result=0 → "注册成功"                                  │
│       ├─ result=1 → "名字已被注册"                              │
│       └─ result=2 → "电话号码已被注册"                          │
└─────────────────────────────────────────────────────────────────┘
```

### 3.3 登录流程

```
┌─────────────────────────────────────────────────────────────────┐
│  客户端                                                         │
│                                                                 │
│  用户点击"登录"按钮                                              │
│       ↓                                                         │
│  LoginDialog::on_pb_login_clicked()                             │
│       ├─ 读取输入：tel, pass                                     │
│       ├─ 校验：非空、tel=11位、pass≤20                          │
│       └─ emit sig_loginData(tel, pass)                          │
│              ↓                                                   │
│  CKernel::slots_loginData()                                     │
│       ├─ 打包 PROT_LOGIN_RQ { packtype=1002, tel, pass }        │
│       └─ m_pMediator->sendData() 发送                           │
│                                                                 │
│  ──────────────────── TCP ──────────────────────────────→       │
│                                                                 │
│  服务端处理                                                      │
│       ├─ SELECT pass, salt, id FROM t_userinfo WHERE tel=?      │
│       ├─ hashPassword(输入密码 + 数据库盐值)                     │
│       ├─ 比对哈希                                                │
│       ├─ 保存 socket 映射                                        │
│       └─ 推送 PROT_FRIEND_INFO × N                              │
│                                                                 │
│  ←──────────────────── TCP ──────────────────────────────       │
│                                                                 │
│  CKernel::slot_delData() → 协议分发                              │
│       ↓                                                         │
│  CKernel::dealLoginRs()    ← 先收到登录回复(1003)                │
│       ├─ result=0: m_id=userId, 隐藏登录窗口, 显示好友列表       │
│       ├─ result=1: "用户不存在"                                  │
│       └─ result=2: "密码错误"                                    │
│       ↓                                                         │
│  CKernel::dealFriendInfo() ← 随后收到多条好友信息(1010)          │
│       ├─ 第一条：自己的信息 → setUserInfo() 显示在好友列表顶部    │
│       └─ 后续每一条：好友信息                                    │
│            ├─ new frienditem                                    │
│            ├─ connect sig_showChatDialog → slot                 │
│            ├─ setFriendInfo() 设置头像、昵称、状态               │
│            ├─ addFriend() 添加到列表                             │
│            ├─ new ChatDialog 预创建聊天窗口                      │
│            └─ connect sig_chatMessage → slot                    │
└─────────────────────────────────────────────────────────────────┘
```

### 3.4 聊天流程

```
发送方 A                              服务端                          接收方 B
────────                              ────                           ────────

用户输入消息，点击"发送"
   ↓
ChatDialog::on_pb_send_clicked()
   ├─ 本地显示消息 + 时间戳
   ├─ 清空输入框
   └─ emit sig_chatMessage(friendId, html)
        ↓
CKernel::slots_chatMessage()
   ├─ 打包 PROT_CHAT_RQ
   │    { packtype=1005, userId=A, friendId=B, content }
   └─ sendData() ──→
                        dealChatRq()
                         ├─ 查找 B 的 socket
                         ├─ 在线 → 转发给 B ──→
                         └─ 离线 → 发送 PROT_CHAT_INFO_RS 给 A
                                                  ↓
                                    CKernel::dealChatRq()
                                     ├─ 拆包取出 content
                                     ├─ m_mapIdToChatDlg[A] 取聊天窗口
                                     ├─ chat->setChatMessage(content)
                                     └─ chat->show()
```

### 3.5 添加好友流程

```
发送方 A                              服务端                          接收方 B
────────                              ────                           ────────

用户点击菜单"添加好友"
   ↓
friendList::slot_menuTrigger()
   └─ emit sig_addFriend()
        ↓
CKernel::slots_addFriend()
   ├─ QInputDialog 输入好友昵称
   ├─ 校验：非空、非自己、非已有好友
   └─ send PROT_ADD_FRIEND_RQ ──→
                                    dealAddFriRq()
                                     ├─ SELECT 查找好友
                                     ├─ 在线 → 转发给 B ──→
                                     └─ 离线 → 返回 OFFLINE 给 A
                                                              ↓
                                                CKernel::dealAddFriRq()
                                                 ├─ QMessageBox: "【A】想添加你为好友，是否同意？"
                                                 ├─ 同意 → result=ACCEPT(0)
                                                 └─ 拒绝 → result=REJECT(2)
                                                 send PROT_ADD_FRIEND_RS ──→
                                                                            │
  ←── dealAddFriRs()                                                        │
   ├─ ACCEPT(0)  → "添加成功"
   ├─ OFFLINE(1) → "好友不在线"
   ├─ REJECT(2)  → "好友拒绝"
   └─ NOTEXIST(3)→ "好友不存在"
```

### 3.6 关闭/下线流程

```
用户关闭好友列表窗口
   ↓
friendList::closeEvent()
   ├─ 拦截关闭事件
   ├─ QMessageBox "是否退出？"
   └─ Yes → emit sig_offline()
              ↓
CKernel::slots_offline()
   ├─ send PROT_FRIEND_OFFLINE { userId } → 通知服务器
   └─ slots_closeProcess()
        ├─ 隐藏/删除 LoginDialog
        ├─ 关闭网络连接
        ├─ 隐藏/删除 friendList
        ├─ 遍历删除所有 ChatDialog
        └─ exit(0)
```

---

## 4. 代码详解

### 4.1 主程序入口

#### main.cpp

```cpp
#include "ckernel.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CKernel kernel;      // 创建核心控制器，启动一切
    return a.exec();     // 进入 Qt 事件循环
}
```

**作用：**
- 创建 Qt 应用程序
- 实例化 CKernel（构造时自动完成所有初始化）
- 进入事件循环等待用户操作

**控制流：** main() 只做三件事，所有逻辑在 CKernel 构造中完成。

---

### 4.2 核心控制器 CKernel

#### ckernel.h — 类定义

```cpp
class CKernel : public QObject
{
    Q_OBJECT
public:
    explicit CKernel(QObject *parent = nullptr);
    ~CKernel();

    void setProtocol();          // 初始化协议处理函数指针表

    // 编码转换
    QString gb2312ToUtf8(char* src);
    void Utf8Togb2312(QString src, char* dst, int len);

    // 8 个协议处理函数（接收方向）
    void dealRegisterRs(...);    // 注册回复 1001
    void dealLoginRs(...);       // 登录回复 1003
    void dealFriendInfo(...);    // 好友信息 1010
    void dealChatRq(...);        // 聊天请求 1005（好友发来的消息）
    void dealChatRs(...);        // 聊天回复 1006（发送结果）
    void dealOfflineRq(...);     // 下线通知 1009
    void dealAddFriRq(...);      // 添加好友请求 1007（别人加我）
    void dealAddFriRs(...);      // 添加好友回复 1008

private slots:
    void slot_delData(char* data, int len, unsigned long from);  // 总入口
    void slots_registerData(QString name, QString tel, QString pass);
    void slots_loginData(QString tel, QString pass);
    void slots_showChatDialog(int friendId);
    void slots_chatMessage(int friendId, QString content);
    void slots_closeProcess();
    void slots_offline();
    void slots_addFriend();

private:
    int m_id;                                       // 当前用户 ID
    QString m_name;                                 // 当前用户昵称
    LoginDialog* m_pLoginDlg;                       // 登录窗口
    INetMediator* m_pMediator;                      // 网络中介
    friendList* m_pFriendList;                      // 好友列表窗口
    PFUN m_protocol[DEF_PROTOCOL_COUNT];            // 函数指针表（15 项）
    QMap<int, frienditem*> m_mapIdToFriendItem;     // 好友 ID → 好友组件
    QMap<int, ChatDialog*> m_mapIdToChatDlg;        // 好友 ID → 聊天窗口
};
```

#### ckernel.cpp — 构造与析构

```cpp
CKernel::CKernel(QObject *parent) : QObject(parent)
{
    setProtocol();          // ① 初始化协议处理函数表

    m_pFriendList = new friendList;   // ② 创建好友列表（隐藏）
    m_pLoginDlg = new LoginDialog;    // ③ 创建登录窗口
    m_pLoginDlg->show();             // ④ 显示登录窗口

    // ⑤ 绑定登录/注册相关的信号槽
    connect(m_pLoginDlg, &LoginDialog::sig_registerData,
            this, &CKernel::slots_registerData);
    connect(m_pLoginDlg, &LoginDialog::sig_loginData,
            this, &CKernel::slots_loginData);
    connect(m_pLoginDlg, &LoginDialog::sig_closeProcess,
            this, &CKernel::slots_closeProcess);
    connect(m_pFriendList, &friendList::sig_offline,
            this, &CKernel::slots_offline);
    connect(m_pFriendList, &friendList::sig_addFriend,
            this, &CKernel::slots_addFriend);

    // ⑥ 创建网络中介并绑定数据接收
    m_pMediator = new TcpClientMediator;
    connect(m_pMediator, &TcpClientMediator::sig_dealData,
            this, &CKernel::slot_delData);

    // ⑦ 连接服务器
    if (!m_pMediator->openNet()) {
        QMessageBox::about(m_pLoginDlg, "提示", "打开网络失败");
        exit(1);
    }
}
```

**构造顺序：** 协议表 → UI 对象 → 信号槽绑定 → 网络连接。这个顺序保证了网络数据到达时，所有处理函数和 UI 都已就绪。

#### ckernel.cpp — 协议分发（核心机制）

```cpp
void CKernel::slot_delData(char *data, int len, unsigned long from)
{
    // 1. 取协议类型（结构体第一个字段必须是 packageType）
    packageType type = *(packageType*)data;

    // 2. 计算函数指针表索引
    int index = type - DEF_PROTOCOL_BASE;  // type - 1000

    // 3. 边界检查 + 调用
    if (index >= 0 && index < DEF_PROTOCOL_COUNT) {
        PFUN pf = m_protocol[index];
        if (pf) {
            (this->*pf)(data, len, from);  // 成员函数指针调用
        }
    }

    // 4. 回收数据内存（由网络层 new[] 分配）
    delete[] data;
}
```

**分发原理：**
```
协议号 1001 ─→ index=1 ─→ m_protocol[1] ─→ dealRegisterRs()
协议号 1003 ─→ index=3 ─→ m_protocol[3] ─→ dealLoginRs()
协议号 1010 ─→ index=10 ─→ m_protocol[10] ─→ dealFriendInfo()
...
```

#### ckernel.cpp — 登录处理（详细）

```cpp
// 发送登录请求
void CKernel::slots_loginData(QString tel, QString pass)
{
    // 打包：结构体构造函数自动设置 packtype=1002
    PROT_LOGIN_RQ rq;
    strcpy_s(rq.pass, sizeof(rq.pass), pass.toStdString().c_str());
    strcpy_s(rq.tel,  sizeof(rq.tel),  tel.toStdString().c_str());

    // 发送（89 是填充的 socket 值，实际由网络层处理）
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

// 处理登录回复
void CKernel::dealLoginRs(char *data, int len, unsigned long from)
{
    PROT_LOGIN_RS* rs = (PROT_LOGIN_RS*)data;
    switch (rs->result) {
    case DEF_LOGIN_SUC:           // 0 — 成功
        m_id = rs->userId;        // 保存自己的用户 ID
        m_pLoginDlg->hide();      // 隐藏登录窗口
        m_pFriendList->show();    // 显示好友列表
        // 后续服务端会推送 FRIEND_INFO，由 dealFriendInfo() 处理
        break;
    case DEF_LOGIN_USERNOTEXIST:  // 1 — 用户不存在
        QMessageBox::about(m_pLoginDlg, "提示", "用户不存在");
        break;
    case DEF_LOGIN_PASSERR:       // 2 — 密码错误
        QMessageBox::about(m_pLoginDlg, "提示", "密码错误");
        break;
    }
}
```

#### ckernel.cpp — 处理好友信息

```cpp
void CKernel::dealFriendInfo(char *data, int len, unsigned long from)
{
    PROT_FRIEND_INFO* info = (PROT_FRIEND_INFO*)data;
    QString name = info->name;
    QString feeling = info->feeling;

    // 判断是否是自己的信息
    if (m_id == info->id) {
        m_name = name;  // 保存自己的昵称
        m_pFriendList->setUserInfo(name, feeling, info->iconId);
        return;
    }

    // 判断是否已在好友列表中
    if (m_mapIdToFriendItem.count(info->id) > 0) {
        // 已存在：更新信息（例如在线状态变化）
        frienditem* item = m_mapIdToFriendItem[info->id];
        item->setFriendInfo(info->id, name, feeling, info->iconId, info->status);
    } else {
        // 新好友：创建组件并添加到列表
        frienditem* item = new frienditem;
        connect(item, &frienditem::sig_showChatDialog,
                this, &CKernel::slots_showChatDialog);
        item->setFriendInfo(info->id, name, feeling, info->iconId, info->status);
        m_pFriendList->addFriend(item);
        m_mapIdToFriendItem[info->id] = item;

        // 预创建一个聊天窗口
        ChatDialog* chat = new ChatDialog;
        chat->setFriendinfo(info->id, name);
        m_mapIdToChatDlg[info->id] = chat;
        connect(chat, &ChatDialog::sig_chatMessage,
                this, &CKernel::slots_chatMessage);
    }
}
```

---

### 4.3 登录对话框 LoginDialog

**文件：** `logindialog.h`, `logindialog.cpp`, `logindialog.ui`

#### 信号声明

```cpp
class LoginDialog : public QDialog
{
    Q_OBJECT
signals:
    void sig_registerData(QString name, QString tel, QString pass);
    void sig_loginData(QString tel, QString pass);
    void sig_closeProcess();
};
```

#### 登录按钮处理

```cpp
void LoginDialog::on_pb_login_clicked()
{
    // 1. 获取控件数据
    QString tel  = ui->le_tel->text();
    QString pass = ui->le_pass->text();

    // 2. 校验合法性
    //    - 不能为空或全空格
    //    - tel 必须为 11 位数字
    //    - pass 不超过 20 字符
    if (tel.isEmpty() || pass.isEmpty() || ...) {
        QMessageBox::about(this, "提示", "输入不能是空或全空格");
        return;
    }
    if (tel.length() != 11 || pass.length() > 20) {
        QMessageBox::about(this, "提示", "电话号码必须是11位，密码不超过20");
        return;
    }

    // 3. 发送信号给 CKernel
    emit sig_loginData(tel, pass);
}
```

#### 注册按钮处理

```cpp
void LoginDialog::on_pb_register_clicked()
{
    QString name = ui->le_register_name->text();
    QString tel  = ui->le_register_tel->text();
    QString pass = ui->le_register_pass->text();

    // 校验：非空、name≤20、tel=11、pass≤20
    // ...

    emit sig_registerData(name, tel, pass);
}
```

#### 关闭事件处理

```cpp
void LoginDialog::closeEvent(QCloseEvent *event)
{
    emit sig_closeProcess();  // 通知 CKernel 清理资源退出
}
```

---

### 4.4 好友列表 FriendList

**文件：** `friendlist.h`, `friendlist.cpp`, `friendlist.ui`

#### 关键方法

```cpp
class friendList : public QWidget
{
    Q_OBJECT
signals:
    void sig_offline();                          // 用户关闭窗口 → 下线
    void sig_addFriend();                         // 用户点击添加好友

public:
    void setUserInfo(QString name, QString feeling, int iconId);
    void addFriend(frienditem* item);
};
```

#### setUserInfo — 显示自己的信息

```cpp
void friendList::setUserInfo(QString name, QString feeling, int iconId)
{
    ui->lb_name->setText(name);          // 设置昵称
    ui->lb_feeling->setText(feeling);    // 设置心情签名

    // 加载对应头像
    QString iconPath = QString(":/tx/%1.png").arg(iconId);
    ui->pb_icon->setIcon(QIcon(iconPath));
}
```

#### addFriend — 将好友添加到列表

```cpp
void friendList::addFriend(frienditem* item)
{
    // 创建垂直布局中的一项
    QListWidgetItem* listItem = new QListWidgetItem;
    listItem->setSizeHint(QSize(290, 80));
    ui->lw_friendList->addItem(listItem);       // 添加到列表控件
    ui->lw_friendList->setItemWidget(listItem, item);  // 嵌入自定义 widget
}
```

#### 关闭窗口 → 下线

```cpp
void friendList::closeEvent(QCloseEvent *event)
{
    event->ignore();  // 拦截默认关闭行为
    if (QMessageBox::Yes ==
        QMessageBox::question(this, "提示", "是否退出？")) {
        emit sig_offline();  // 通知 CKernel 发送下线协议
    }
}
```

#### 添加好友菜单

```cpp
// 菜单点击后根据 action 文本判断操作
void friendList::slot_menuTrigger(QAction* action)
{
    if (action->text() == "添加好友") {
        emit sig_addFriend();
    }
    // 可扩展其他菜单项...
}
```

---

### 4.5 好友组件 FriendItem

**文件：** `frienditem.h`, `frienditem.cpp`, `frienditem.ui`

#### 组件构成

```
┌──────────────────────────────────────────┐
│  ┌────────┐                              │
│  │ 头像   │  昵称                 状态   │
│  │ pb_icon│  lb_name          lb_status │
│  └────────┘  心情签名                   │
│              lb_feeling                 │
└──────────────────────────────────────────┘
```

#### 关键方法

```cpp
class frienditem : public QWidget
{
    Q_OBJECT
signals:
    void sig_showChatDialog(int friendId);  // 点击头像 → 打开聊天窗口

public:
    void setFriendInfo(int id, QString name, QString feeling,
                       int iconId, int status);
    void setFriendOffline();               // 设置离线状态
    const QString& name() const;           // 获取好友昵称
};
```

#### setFriendInfo — 设置好友信息

```cpp
void frienditem::setFriendInfo(int id, QString name, QString feeling,
                                int iconId, int status)
{
    m_id = id;
    m_name = name;
    ui->lb_name->setText(name);
    ui->lb_feeling->setText(feeling);

    // 加载头像
    QString iconPath = QString(":/tx/%1.png").arg(iconId);
    ui->pb_icon->setIcon(QIcon(iconPath));

    // 在线状态：彩色头像 vs 灰色头像
    if (status == DEF_STATUS_ONLINE) {
        ui->pb_icon->setEnabled(true);
        ui->lb_status->setText("在线");
    } else {
        ui->pb_icon->setEnabled(false);
        ui->lb_status->setText("离线");
    }
}
```

#### 点击头像 → 打开聊天

```cpp
void frienditem::on_pb_icon_clicked()
{
    emit sig_showChatDialog(m_id);  // 传递好友 ID
}
```

---

### 4.6 聊天对话框 ChatDialog

**文件：** `chatdialog.h`, `chatdialog.cpp`, `chatdialog.ui`

#### 布局

```
┌──────────────────────────────────────────┐
│  好友昵称                                │
├──────────────────────────────────────────┤
│                                          │
│  QTextBrowser (tb_chat)                  │
│  聊天记录显示区                           │
│                                          │
├──────────────────────────────────────────┤
│  ┌──────────────────────────────┐        │
│  │ QTextEdit (te_message)       │ [发送] │
│  │ 消息输入区                    │        │
│  └──────────────────────────────┘        │
└──────────────────────────────────────────┘
```

#### 关键方法

```cpp
class ChatDialog : public QWidget
{
    Q_OBJECT
signals:
    void sig_chatMessage(int friendId, QString content);  // 发送消息

public:
    void setFriendinfo(int id, QString name);
    void setChatMessage(QString content);     // 接收并显示消息
    void setFriOffline();                     // 显示对方离线提示
};
```

#### 发送消息

```cpp
void ChatDialog::on_pb_send_clicked()
{
    QString content = ui->te_message->toHtml();  // 支持 HTML 格式

    if (content.isEmpty()) return;

    // 本地立即显示（自己发出的消息）
    QString timestr = QTime::currentTime().toString("hh:mm:ss");
    ui->tb_chat->append(QString("I %1").arg(timestr));
    ui->tb_chat->append(content);

    // 清空输入框
    ui->te_message->clear();

    // 发送信号给 CKernel
    emit sig_chatMessage(m_friendId, content);
}
```

#### 显示别人发来的消息

```cpp
void ChatDialog::setChatMessage(QString content)
{
    QString timestr = QTime::currentTime().toString("hh:mm:ss");
    ui->tb_chat->append(QString("[%1] %2").arg(m_friendName, timestr));
    ui->tb_chat->append(content);
}
```

---

### 4.7 网络层 Net

#### def.h — 协议定义

**文件：** `net/def.h`

##### 常量定义

```cpp
#define _DEF_TCP_PORT       (67890)    // 服务器端口
#define _DEF_UDP_PORT       (12345)    // UDP 端口（预留）
#define DEF_INFO_LEN        (20)       // 基本信息字段长度
#define DEF_CHATMSG_LEN     (1024*8)   // 聊天内容最大 8KB
#define DEF_PROTOCOL_BASE   1000       // 协议号基准
#define DEF_PROTOCOL_COUNT  15         // 函数指针表容量
```

##### 协议号定义

```cpp
// 注册：请求 1000，回复 1001
#define DEF_REGISTER_RQ     (1000)
#define DEF_REGISTER_RS     (1001)

// 登录：请求 1002，回复 1003
#define DEF_LOGIN_RQ        (1002)
#define DEF_LOGIN_RS        (1003)

// 聊天：请求 1005，回复 1006
#define DEF_CHATMSG_RQ      (1005)
#define DEF_CHATMSG_RS      (1006)

// 添加好友：请求 1007，回复 1008
#define DEF_ADD_FRIEND_RQ   (1007)
#define DEF_ADD_FRIEND_RS   (1008)

// 下线通知：1009
#define DEF_FRIEND_OFFLINE  (1009)

// 好友信息推送：1010
#define DEF_FRIEND_INFO     (1010)
```

##### 结果码定义

```cpp
// 注册结果
#define DEF_REGISTER_SUC          (0)  // 成功
#define DEF_REGISTER_NAME_EXIST   (1)  // 用户名已存在
#define DEF_REGISTER_TEL_EXIST    (2)  // 电话已存在

// 登录结果
#define DEF_LOGIN_SUC             (0)  // 成功
#define DEF_LOGIN_USERNOTEXIST    (1)  // 用户不存在
#define DEF_LOGIN_PASSERR         (2)  // 密码错误

// 添加好友结果
#define DEF_ADD_FRIEND_ACCEPT     (0)  // 接受
#define DEF_ADD_FRIEND_OFFLINE    (1)  // 离线
#define DEF_ADD_FRIEND_REJECT     (2)  // 拒绝
#define DEF_ADD_FRIEND_NOTEXIST   (3)  // 不存在

// 用户状态
#define DEF_STATUS_ONLINE         (0)
#define DEF_STATUS_OFFLINE        (1)
```

#### TCPClient.cpp — 原始 Socket 实现

##### 连接服务器

```cpp
bool TCPClient::initNet()
{
    // 1. 加载 Winsock2 库
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. 创建 TCP socket
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 3. 连接服务器
    struct sockaddr_in addrServer;
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(_DEF_TCP_PORT);          // 67890
    addrServer.sin_addr.S_un.S_addr = inet_addr("192.168.134.128");
    connect(m_sock, (struct sockaddr*)&addrServer, sizeof(addrServer));

    // 4. 启动接收线程
    _beginthreadex(NULL, 0, &TCPClient::recvData, this, 0, NULL);
    return true;
}
```

##### 发送数据（4 字节长度前缀 + 负载）

```cpp
bool TCPClient::sendData(char* data, int len, unsigned long to)
{
    // 1. 先发送 4 字节长度
    send(m_sock, (char*)&len, sizeof(int), 0);

    // 2. 再发送数据
    int totalSent = 0;
    while (len > 0) {
        int n = send(m_sock, data + totalSent, len, 0);
        totalSent += n;
        len -= n;
    }
    return true;
}
```

##### 接收数据（循环读取直到完整）

```cpp
DWORD TCPClient::recvData(LPVOID lpParameter)
{
    TCPClient* pThis = (TCPClient*)lpParameter;

    while (pThis->m_bRun) {
        // 1. 读取 4 字节长度
        int packLen = 0;
        recv(pThis->m_sock, (char*)&packLen, sizeof(int), 0);

        // 2. 分配缓冲区并循环读取
        char* packBuf = new char[packLen];
        int offset = 0;
        int remaining = packLen;

        while (remaining > 0) {
            int n = recv(pThis->m_sock, packBuf + offset, remaining, 0);
            if (n > 0) {
                offset += n;
                remaining -= n;
            } else if (n == 0) {
                // 服务器断开连接
                delete[] packBuf;
                pThis->m_pMediator->disConnect();
                return 0;
            }
        }

        // 3. 转发给中介层
        pThis->m_pMediator->transmateData(packBuf, packLen, pThis->m_sock);
        // 注意：packBuf 由上层 CKernel::slot_delData() 负责 delete[]
    }
    return 0;
}
```

---

### 4.8 中介层 Mediator

#### INetMediator — 抽象接口

```cpp
class INetMediator : public QObject
{
    Q_OBJECT
public:
    virtual bool openNet() = 0;
    virtual bool closeNet() = 0;
    virtual bool sendData(char* data, int len, unsigned long to) = 0;
    virtual void transmateData(char* data, int len, unsigned long from) = 0;
    virtual void disConnect() = 0;

signals:
    void sig_dealData(char* data, int len, unsigned long from);  // 通知 CKernel
    void sig_disConnect();                                        // 断线通知
};
```

#### TcpClientMediator — TCP 客户端中介

```cpp
class TcpClientMediator : public INetMediator
{
    Q_OBJECT
private:
    TCPClient* m_pNet;
    bool m_bConnected;
};
```

```cpp
// 打开网络：创建 TCPClient 并初始化
bool TcpClientMediator::openNet()
{
    m_pNet = new TCPClient;
    m_pNet->setMediator(this);          // 设置回调目标

    if (!m_pNet->initNet()) {
        return false;
    }
    m_bConnected = true;
    return true;
}

// 发送数据（带自动重连）
bool TcpClientMediator::sendData(char* data, int len, unsigned long to)
{
    if (!m_bConnected) {
        // 断线重连
        m_pNet->unInitNet();
        if (!m_pNet->initNet()) return false;
        m_bConnected = true;
    }
    return m_pNet->sendData(data, len, to);
}

// 数据从网络层到达 → 发射信号给 CKernel
void TcpClientMediator::transmateData(char* data, int len, unsigned long from)
{
    emit sig_dealData(data, len, from);
}

// 断线处理
void TcpClientMediator::disConnect()
{
    m_bConnected = false;
    emit sig_disConnect();
}
```

---

## 5. 通信协议

### 5.1 协议结构

```
┌─────────────────────────────────────┐
│         网络数据包格式              │
├─────────────────────────────────────┤
│  [4字节: 数据包长度 (int)]          │
│  [N字节: 数据包内容]                │
└─────────────────────────────────────┘

数据包内容:
┌─────────────────────────────────────┐
│  [4字节: 协议类型 (packageType)]    │
│  [其他: 协议数据]                   │
└─────────────────────────────────────┘
```

### 5.2 协议号定义

| 协议号 | 宏名 | 方向 | 结构体 |
|--------|------|------|--------|
| 1000 | DEF_REGISTER_RQ | 客户端 → 服务端 | PROT_REGISTER_RQ |
| 1001 | DEF_REGISTER_RS | 服务端 → 客户端 | PROT_REGISTER_RS |
| 1002 | DEF_LOGIN_RQ | 客户端 → 服务端 | PROT_LOGIN_RQ |
| 1003 | DEF_LOGIN_RS | 服务端 → 客户端 | PROT_LOGIN_RS |
| 1005 | DEF_CHATMSG_RQ | 双向 | PROT_CHAT_RQ |
| 1006 | DEF_CHATMSG_RS | 服务端 → 客户端 | PROT_CHAT_INFO_RS |
| 1007 | DEF_ADD_FRIEND_RQ | 双向（服务端转发） | PROT_ADD_FRIEND_RQ |
| 1008 | DEF_ADD_FRIEND_RS | 双向（服务端转发） | PROT_ADD_FRIEND_RS |
| 1009 | DEF_FRIEND_OFFLINE | 客户端 → 服务端 / 服务端 → 客户端 | PROT_FRIEND_OFFLINE |
| 1010 | DEF_FRIEND_INFO | 服务端 → 客户端 | PROT_FRIEND_INFO |

### 5.3 结构体详情

#### 注册请求 PROT_REGISTER_RQ (1000, 64 字节)

```cpp
struct PROT_REGISTER_RQ {
    packageType packtype;     // 4 字节 = 1000
    char name[20];           // 用户名
    char tel[20];            // 电话
    char pass[20];           // 密码（明文）
};
```

#### 注册回复 PROT_REGISTER_RS (1001, 8 字节)

```cpp
struct PROT_REGISTER_RS {
    packageType packtype;    // 4 字节 = 1001
    int result;              // 0=成功, 1=名字已存在, 2=电话已存在
};
```

#### 登录请求 PROT_LOGIN_RQ (1002, 44 字节)

```cpp
struct PROT_LOGIN_RQ {
    packageType packtype;    // 4 字节 = 1002
    char tel[20];           // 电话
    char pass[20];          // 密码（明文）
};
```

#### 登录回复 PROT_LOGIN_RS (1003, 12 字节)

```cpp
struct PROT_LOGIN_RS {
    packageType packtype;    // 4 字节 = 1003
    int userId;             // 服务器分配的用户 ID
    int result;             // 0=成功, 1=用户不存在, 2=密码错误
};
```

#### 聊天请求 PROT_CHAT_RQ (1005, ~8KB)

```cpp
struct PROT_CHAT_RQ {
    packageType packtype;    // 4 字节 = 1005
    int userId;             // 发送者 ID
    int friendId;           // 接收者 ID
    char content[8192];     // 聊天内容（HTML 格式）
};
```

#### 添加好友请求 PROT_ADD_FRIEND_RQ (1007, 52 字节)

```cpp
struct PROT_ADD_FRIEND_RQ {
    packageType packtype;    // 4 字节 = 1007
    int userId;             // 我的 ID
    char username[20];      // 我的昵称
    char friendName[20];    // 要添加的好友昵称
};
```

#### 添加好友回复 PROT_ADD_FRIEND_RS (1008, 56 字节)

```cpp
struct PROT_ADD_FRIEND_RS {
    packageType packtype;    // 4 字节 = 1008
    int result;             // 0=接受, 1=离线, 2=拒绝, 3=不存在
    int userId;             // 发送者 ID
    int friendId;           // 接收者 ID
    char userName[20];      // 发送者昵称
    char friendName[20];    // 接收者昵称
};
```

#### 好友信息推送 PROT_FRIEND_INFO (1010, ~90 字节)

```cpp
struct PROT_FRIEND_INFO {
    packageType packtype;    // 4 字节 = 1010
    int id;                 // 用户 ID
    int iconId;             // 头像编号 (0-35)
    int status;             // 状态：0=在线, 1=离线
    char name[20];          // 昵称
    char feeling[50];       // 心情签名
};
```

---

## 6. 信号与槽机制

### 6.1 完整连接图

```
                        CKernel
                      (核心控制器)

  LoginDialog ──sig_registerData──→ slots_registerData → 打包发送 1000
              ──sig_loginData─────→ slots_loginData    → 打包发送 1002
              ──sig_closeProcess──→ slots_closeProcess → 清理退出

  friendList  ──sig_offline───────→ slots_offline      → 发送 1009 + 退出
              ──sig_addFriend─────→ slots_addFriend    → 输入框 → 发送 1007

  frienditem  ──sig_showChatDialog→ slots_showChatDialog → chat->show()

  ChatDialog  ──sig_chatMessage───→ slots_chatMessage  → 打包发送 1005

  TcpMediator ──sig_dealData──────→ slot_delData       → 协议分发到 8 个处理函数
              ──sig_disConnect────→ (处理断线重连)
```

### 6.2 连接清单

| # | 信号 | 槽 | 绑定位置 |
|---|------|----|---------|
| 1 | LoginDialog::sig_registerData | CKernel::slots_registerData | ckernel.cpp:18 |
| 2 | LoginDialog::sig_loginData | CKernel::slots_loginData | ckernel.cpp:20 |
| 3 | LoginDialog::sig_closeProcess | CKernel::slots_closeProcess | ckernel.cpp:22 |
| 4 | friendList::sig_offline | CKernel::slots_offline | ckernel.cpp:24 |
| 5 | friendList::sig_addFriend | CKernel::slots_addFriend | ckernel.cpp:26 |
| 6 | TcpClientMediator::sig_dealData | CKernel::slot_delData | ckernel.cpp:31 |
| 7* | frienditem::sig_showChatDialog | CKernel::slots_showChatDialog | ckernel.cpp:179 |
| 8* | ChatDialog::sig_chatMessage | CKernel::slots_chatMessage | ckernel.cpp:195 |

> \* 连接 7 和 8 在 `dealFriendInfo()` 中为每个好友动态创建。

### 6.3 Qt 自动连接（按名称约定）

| # | 控件 | 槽函数 |
|---|------|--------|
| 9 | pb_register::clicked() | LoginDialog::on_pb_register_clicked() |
| 10 | pb_login::clicked() | LoginDialog::on_pb_login_clicked() |
| 11 | pb_login_clear::clicked() | LoginDialog::on_pb_login_clear_clicked() |
| 12 | pb_clear::clicked() | LoginDialog::on_pb_clear_clicked() |
| 13 | pb_send::clicked() | ChatDialog::on_pb_send_clicked() |
| 14 | pb_icon::clicked() | frienditem::on_pb_icon_clicked() |
| 15 | pb_menu::clicked() | friendList::on_pb_menu_clicked() |

---

## 7. 编码转换

### 7.1 为什么需要编码转换

```
Qt 内部：UTF-8 (QString)
网络传输：GB2312 (char*)
服务端/数据库：UTF-8 (已升级为 utf8mb4)
```

客户端在 Qt 和网络之间做 GB2312 ↔ UTF-8 转换。

### 7.2 转换函数

```cpp
// GB2312 网络数据 → UTF-8 Qt 显示
QString CKernel::gb2312ToUtf8(char *src)
{
    QTextCodec* dc = QTextCodec::codecForName("gb2312");
    return dc->toUnicode(src);
}

// UTF-8 Qt 输入 → GB2312 网络发送
void CKernel::Utf8Togb2312(QString src, char *dst, int len)
{
    QTextCodec* dc = QTextCodec::codecForName("gb2312");
    QByteArray ba = dc->fromUnicode(src);
    strcpy_s(dst, len, ba.data());
}
```

### 7.3 使用场景

| 场景 | 转换 | 代码位置 |
|------|------|---------|
| 注册/登录发数据 | strcpy_s 直接复制（ASCII 兼容部分） | slots_registerData / slots_loginData |
| 添加好友（好友名） | Utf8Togb2312(name → rq.friendName) | ckernel.cpp:473 |
| 接收添加好友回复 | gb2312ToUtf8(rs->friendName) | ckernel.cpp:279 |

> **注意：** 服务端已升级为 utf8mb4，如果客户端发送的是 GB2312 编码的中文，服务端直接存入 utf8mb4 数据库可能产生乱码。需要统一编码策略。

---

## 📝 附录

### A. 编译和运行

```bash
# Windows + Qt Creator
# 1. 打开 IMClient.pro
# 2. 选择 Kit: Desktop Qt 5.12.11 MinGW 32-bit
# 3. 构建 → 运行

# 客户端可执行文件
build-IMClient-Desktop_Qt_5_12_11_MinGW_32_bit-Debug/debug/IMClient.exe
```

### B. 服务器连接配置

| 配置项 | 值 | 位置 |
|--------|----|------|
| 服务器 IP | 192.168.134.128 | net/TCPClient.cpp:62 |
| 端口 | 67890 | net/def.h:10 |

### C. 资源文件

| 目录 | 内容 | 数量 |
|------|------|------|
| `tx/` | 用户头像 (0.png ~ 35.png) | 36 个 |
| `images/` | UI 工具栏图标 | 27 个 |

### D. 关键设计模式

| 模式 | 应用 |
|------|------|
| **单例** | CKernel 只有一个实例 |
| **中介者** | Mediator 层隔离 UI 和原始 Socket |
| **函数指针表** | 协议分发 (类似 virtual dispatch) |
| **观察者** | Qt 信号槽机制 |

---

## ✅ 文档结束

本文档详细解释了 IM 客户端的架构、注册/登录流程、协议分发、信号槽机制和编码转换。

**与服务器文档配套使用：** `E:\COLIN\Linux\share\IM服务器开发文档.md`
