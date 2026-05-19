#pragma once

#include <QMap>
#include <QByteArray>
#include <QMutex>
#include <openssl/sha.h>
#include "cmysql.h"
#include "def.h"

// 前向声明
class INetMediator;

class CKernel {
public:
    CKernel();
    ~CKernel();

    void setProtocol();
    bool startServer();
    void endServer();
    void dealData(char* data, int len, unsigned long from);

    //处理注册请求
    void dealRegisterRq(char* data, int len, unsigned long from);
    //处理登录请求
    void dealLoginRq(char* data, int len, unsigned long from);

    //获取用户信息
    void getUserInfoAddFriendInfo(int userId);

    //处理聊天请求
    void dealChatRq(char* data, int len, unsigned long from);

    //处理下线请求
    void dealOfflineRq(char* data, int len, unsigned long from);

    //处理添加好友请求
    void dealAddFriRq(char* data, int len, unsigned long from);

    //处理添加好友回复
    void dealAddFriRs(char* data, int len, unsigned long from);

    // Room handlers
    void dealRoomCreateRq(char* data, int len, unsigned long from);
    void dealRoomJoinRq(char* data, int len, unsigned long from);
    void dealRoomLeaveRq(char* data, int len, unsigned long from);
    void dealRoomChatRq(char* data, int len, unsigned long from);
    void dealAudioFrame(char* data, int len, unsigned long from);
    void dealVideoFrame(char* data, int len, unsigned long from);
    void dealRoomListRq(char* data, int len, unsigned long from);
    void dealRoomEndRq(char* data, int len, unsigned long from);

    // Audio/video frame forwarding

    //根据id查询用户信息
    void getInfoById(int id, char* info);

    // 移除用户映射（当客户端断开时调用）
    void removeUserBySocket(unsigned long socket);

    static CKernel* pKernel;

private:
    // 密码哈希
    QByteArray hashPassword(const QByteArray& password);
    QByteArray generateSalt();

    // 转义SQL字符串
    QByteArray escapeString(const QByteArray& input);

    INetMediator* m_pMediator;
    CMySql m_sql;
    //协议处理函数指针数组
    typedef void(CKernel::* PFUN)(char* data, int len, unsigned long from);
    PFUN m_protocol[DEF_PROTOCOL_COUNT];
    //登录成功的用户socket（用户id -> socket）
    QMap<int, int> m_mapIdToSocket;
    // socket -> userId 映射（用于客户端断开时查找）
    QMap<int, int> m_mapSocketToId;
    //线程安全锁
    QMutex m_mutex;
};
