#include "ckernel.h"
#include "tcpservermediator.h"
#include <QDebug>
#include <cstring>
#include <unistd.h>
#include <QRandomGenerator>
#include <QTextCodec>

CKernel* CKernel::pKernel = nullptr;

CKernel::CKernel() : m_pMediator(nullptr) {
    pKernel = this;
    setProtocol();
}

CKernel::~CKernel() {
    QMutexLocker locker(&m_mutex);
}

void CKernel::setProtocol() {
    qDebug() << __func__;
    memset(m_protocol, 0, sizeof(m_protocol));
    m_protocol[DEF_REGISTER_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRegisterRq;
    m_protocol[DEF_LOGIN_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealLoginRq;
    m_protocol[DEF_CHATMSG_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealChatRq;
    m_protocol[DEF_FRIEND_OFFLINE - DEF_PROTOCOL_BASE] = &CKernel::dealOfflineRq;
    m_protocol[DEF_ADD_FRIEND_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealAddFriRq;
    m_protocol[DEF_ADD_FRIEND_RS - DEF_PROTOCOL_BASE] = &CKernel::dealAddFriRs;
    m_protocol[DEF_ROOM_CREATE_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomCreateRq;
    m_protocol[DEF_ROOM_JOIN_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomJoinRq;
    m_protocol[DEF_ROOM_LEAVE_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomLeaveRq;
    m_protocol[DEF_ROOM_CHAT_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomChatRq;
    m_protocol[DEF_ROOM_LIST_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomListRq;
    m_protocol[DEF_ROOM_END_RQ - DEF_PROTOCOL_BASE] = &CKernel::dealRoomEndRq;
    m_protocol[DEF_AUDIO_FRAME - DEF_PROTOCOL_BASE] = &CKernel::dealAudioFrame;
    m_protocol[DEF_VIDEO_FRAME - DEF_PROTOCOL_BASE] = &CKernel::dealVideoFrame;
}

bool CKernel::startServer() {
    qDebug() << __func__;
    m_pMediator = new TcpServerMediator;

    if (!m_pMediator->openNet()) {
        qDebug() << "打开网络失败";
        return false;
    }

    // 启动epoll事件循环
    m_pMediator->startRecv();

    char ip[] = "127.0.0.1";
    char user[] = "root";
    char pass[] = "041206";
    char db[] = "20240919im";

    if (!m_sql.ConnectMySql(ip, user, pass, db, 3306)) {
        qDebug() << "打开数据库失败";
        return false;
    }

    return true;
}

void CKernel::endServer() {
    if (m_pMediator) {
        m_pMediator->closeNet();
        delete m_pMediator;
        m_pMediator = nullptr;
    }
}

void CKernel::dealData(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    if (len < (int)sizeof(packageType)) { delete[] data; return; }
    packageType type = *(packageType*)data;

    int index = type - DEF_PROTOCOL_BASE;
    if (index >= 0 && index < DEF_PROTOCOL_COUNT) {
        PFUN pf = m_protocol[index];
        if (pf) {
            (this->*pf)(data, len, from);
        } else {
            qDebug() << "type2:" << type;
        }
    } else {
        qDebug() << "type1:" << type;
    }

    delete[] data;
}

QByteArray CKernel::generateSalt() {
    QByteArray salt;
    salt.resize(32);
    for (int i = 0; i < 32; ++i) {
        int val = QRandomGenerator::global()->bounded(256);
        salt[i] = "0123456789abcdef"[val >> 4];
        ++i;
        salt[i] = "0123456789abcdef"[val & 0xf];
    }
    return salt;
}

QByteArray CKernel::hashPassword(const QByteArray& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.constData(), password.size(), hash);

    return QByteArray::fromRawData((char*)hash, SHA256_DIGEST_LENGTH).toHex();
    // toHex() returns lowercase hex string (64 chars)
}

QByteArray CKernel::escapeString(const QByteArray& input) {
    if (input.isEmpty()) return "";
    QVector<char> buf(input.size() * 2 + 1);
    if (m_sql.EscapeString(input.constData(), buf.data(), buf.size())) {
        return QByteArray(buf.data());
    }
    return "";
}

void CKernel::removeUserBySocket(unsigned long socket) {
    QMutexLocker locker(&m_mutex);

    if (m_mapSocketToId.contains((int)socket)) {
        int userId = m_mapSocketToId[(int)socket];
        m_mapIdToSocket.remove(userId);
        m_mapSocketToId.remove((int)socket);
        qDebug() << "User" << userId << "removed from socket" << socket;
    }
}

void CKernel::dealRegisterRq(char* data, int len, unsigned long from) {
    qDebug() << "========== 注册请求 ==========";
    qDebug() << "收到客户端数据 from socket:" << from;

    PROT_REGISTER_RQ* rq = (PROT_REGISTER_RQ*)data;

    qDebug() << "协议类型:" << rq->packtype;
    qDebug() << "用户名:" << rq->name;
    qDebug() << "电话:" << rq->tel;
    qDebug() << "密码:" << rq->pass;
    qDebug() << "数据长度:" << len << "bytes";
    qDebug() << "================================";

    QByteArray safeName = escapeString(rq->name);
    QByteArray safeTel = escapeString(rq->tel);

    if (safeName.isEmpty() || safeTel.isEmpty()) {
        PROT_REGISTER_RS rs;
        rs.result = 3;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    if (safeName.size() > DEF_INFO_LEN - 1 || safeTel.size() > DEF_INFO_LEN - 1) {
        PROT_REGISTER_RS rs;
        rs.result = 3;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    char checkSql[256];
    snprintf(checkSql, sizeof(checkSql), "SELECT tel FROM t_userinfo WHERE tel = '%s'", safeTel.constData());
    QStringList checkLst;
    if (m_sql.SelectMySql(checkSql, 1, checkLst) && checkLst.size() > 0) {
        PROT_REGISTER_RS rs;
        rs.result = DEF_REGISTER_TEL_EXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    snprintf(checkSql, sizeof(checkSql), "SELECT tel FROM t_userinfo WHERE name = '%s'", safeName.constData());
    checkLst.clear();
    if (m_sql.SelectMySql(checkSql, 1, checkLst) && checkLst.size() > 0) {
        PROT_REGISTER_RS rs;
        rs.result = DEF_REGISTER_NAME_EXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    QByteArray salt = generateSalt();
    QByteArray hashedPassword = hashPassword(QByteArray(rq->pass) + salt);

    qDebug() << "生成盐值:" << salt << "(长度:" << salt.size() << ")";
    qDebug() << "哈希密码:" << hashedPassword << "(长度:" << hashedPassword.size() << ")";

    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO t_userinfo (name, tel, pass, salt, iconid, feeling) VALUES ('%s', '%s', '%s', '%s', 8, '在线')",
             safeName.constData(), safeTel.constData(), hashedPassword.constData(), salt.constData());

    qDebug() << "执行SQL:" << sql;

    if (!m_sql.UpdateMySql(sql)) {
        qDebug() << "插入数据库失败";
        return;
    }
    qDebug() << "INSERT执行成功！";

    PROT_REGISTER_RS rs;
    rs.result = DEF_REGISTER_SUC;
    m_pMediator->sendData((char*)&rs, sizeof(rs), from);
}

void CKernel::dealLoginRq(char* data, int len, unsigned long from) {
    qDebug() << "========== 登录请求 ==========";
    qDebug() << "收到客户端数据 from socket:" << from;

    PROT_LOGIN_RQ* rq = (PROT_LOGIN_RQ*)data;

    qDebug() << "协议类型:" << rq->packtype;
    qDebug() << "电话:" << rq->tel;
    qDebug() << "密码:" << rq->pass;
    qDebug() << "数据长度:" << len << "bytes";
    qDebug() << "================================";

    QByteArray safeTel = escapeString(rq->tel);

    if (safeTel.isEmpty() || safeTel.size() > DEF_INFO_LEN - 1) {
        PROT_LOGIN_RS rs;
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    PROT_LOGIN_RS rs;

    char sql[1024];
    snprintf(sql, sizeof(sql), "SELECT pass, salt, id FROM t_userinfo WHERE tel = '%s'", safeTel.constData());
    QStringList lstStr;

    qDebug() << "执行查询SQL:" << sql;

    if (!m_sql.SelectMySql(sql, 3, lstStr)) {
        qDebug() << "查询数据库失败";
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    if (lstStr.size() < 3) {
        qDebug() << "未找到用户";
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    QByteArray storedHash = lstStr.first().toUtf8();
    lstStr.removeFirst();
    QByteArray salt = lstStr.first().toUtf8();
    lstStr.removeFirst();
    QString userIdStr = lstStr.first();
    lstStr.removeFirst();

    int userId = 0;
    bool ok;
    userId = userIdStr.toInt(&ok);
    if (!ok) {
        qDebug() << "转换用户ID失败:" << userIdStr;
        rs.result = DEF_LOGIN_USERNOTEXIST;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    QByteArray inputHash = hashPassword(QByteArray(rq->pass) + salt);
    qDebug() << "输入密码哈希:" << inputHash.left(20) << "...";

    if (inputHash == storedHash) {
        qDebug() << "密码验证成功！";
        rs.result = DEF_LOGIN_SUC;
        rs.userId = userId;

        {
            QMutexLocker locker(&m_mutex);

            // Check if user is already logged in on another client
            if (m_mapIdToSocket.contains(userId)) {
                int oldSocket = m_mapIdToSocket[userId];
                qDebug() << "User" << userId << "already logged in on socket=" << oldSocket
                         << ", rejecting new login from socket=" << from;

                PROT_LOGIN_RS failRs;
                failRs.result = 3;  // Account already logged in
                failRs.userId = userId;
                m_pMediator->sendData((char*)&failRs, sizeof(failRs), from);
                return;
            }

            // Map new connection to user
            m_mapIdToSocket[userId] = from;
            m_mapSocketToId[(int)from] = userId;
            qDebug() << "User" << userId << "login success, socket=" << from;
        }


        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        getUserInfoAddFriendInfo(userId);
        return;
    } else {
        qDebug() << "密码错误！";
        rs.result = DEF_LOGIN_PASSERR;
    }

    m_pMediator->sendData((char*)&rs, sizeof(rs), from);
}

void CKernel::getUserInfoAddFriendInfo(int userId) {
    qDebug() << __func__;
    PROT_FRIEND_INFO userInfo;
    getInfoById(userId, (char*)&userInfo);

    {
        QMutexLocker locker(&m_mutex);
        qDebug() << "检查当前用户是否在线 userId=" << userId;
        if (m_mapIdToSocket.contains(userId)) {
            qDebug() << "userInfo.feeling:" << userInfo.feeling << "userInfo.name:" << userInfo.name;
            qDebug() << "发送自己的信息到 socket=" << m_mapIdToSocket[userId];
            m_pMediator->sendData((char*)&userInfo, sizeof(userInfo), m_mapIdToSocket[userId]);
        } else {
            qDebug() << "当前用户不在线";
        }
    }

    char friendSql[512];
    snprintf(friendSql, sizeof(friendSql), "SELECT idB FROM t_friend WHERE idA = %d", userId);
    qDebug() << "执行SQL:" << friendSql;
    QStringList friendLst;
    if (m_sql.SelectMySql(friendSql, 1, friendLst)) {
        qDebug() << "找到好友数量:" << friendLst.size();
        int friendId = 0;
        PROT_FRIEND_INFO friendInfo;

        while (friendLst.size() > 0) {
            bool ok;
            friendId = friendLst.first().toInt(&ok);
            friendLst.removeFirst();
            if (!ok) continue;

            getInfoById(friendId, (char*)&friendInfo);

            {
                QMutexLocker locker(&m_mutex);
                qDebug() << "发送好友" << friendId << "信息给用户" << userId;
                if (m_mapIdToSocket.contains(userId)) {
                    qDebug() << "  -> 发送到 socket=" << m_mapIdToSocket[userId];
                    m_pMediator->sendData((char*)&friendInfo, sizeof(friendInfo), m_mapIdToSocket[userId]);
                } else {
                    qDebug() << "  -> 用户不在线，跳过";
                }

                qDebug() << "通知好友" << friendId << "用户" << userId << "上线";
                if (m_mapIdToSocket.contains(friendId)) {
                    qDebug() << "  -> 发送到自己 socket=" << m_mapIdToSocket[friendId];
                    m_pMediator->sendData((char*)&userInfo, sizeof(userInfo), m_mapIdToSocket[friendId]);
                } else {
                    qDebug() << "  -> 好友不在线，跳过";
                }
            }
        }
        qDebug() << "好友通知完成";
    } else {
        qDebug() << "查询好友列表失败";
    }
}

void CKernel::dealChatRq(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_CHAT_RQ* rq = (PROT_CHAT_RQ*)data;

    {
        QMutexLocker locker(&m_mutex);
        if (m_mapIdToSocket.contains(rq->friendId)) {
            m_pMediator->sendData(data, len, m_mapIdToSocket[rq->friendId]);
        } else {
            PROT_CHAT_INFO_RS rs;
            rs.friendId = rq->friendId;
            m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        }
    }
}

void CKernel::dealOfflineRq(char* data, int len, unsigned long from) {
    PROT_FRIEND_OFFLINE* rq = (PROT_FRIEND_OFFLINE*)data;
    qDebug() << "[DEBUG dealOfflineRq] userId:" << rq->userId;

    char offlineSql[256];
    snprintf(offlineSql, sizeof(offlineSql), "SELECT idB FROM t_friend WHERE idA = %d", rq->userId);
    QStringList offlineLst;
    if (m_sql.SelectMySql(offlineSql, 1, offlineLst)) {
        int friendId = 0;
        while (offlineLst.size() > 0) {
            bool ok;
            friendId = offlineLst.first().toInt(&ok);
            offlineLst.removeFirst();
            if (!ok) continue;

            {
                QMutexLocker locker(&m_mutex);
                if (m_mapIdToSocket.contains(friendId)) {
                    m_pMediator->sendData(data, len, m_mapIdToSocket[friendId]);
                }
            }
        }
    }

    {
        int sock = -1;
        {
            QMutexLocker locker(&m_mutex);
            if (m_mapIdToSocket.contains(rq->userId)) {
                sock = m_mapIdToSocket[rq->userId];
            }
        }
        if (sock != -1) {
            // 释放锁后调用：removeClient会回调removeUserBySocket（取同一锁）
            m_pMediator->closeClientSocket((unsigned long)sock);
        }
    }

    // Destroy all rooms created by this user
    {
        char sql[1024];
        snprintf(sql, sizeof(sql),
                 "SELECT roomId FROM t_room WHERE creatorId = %d", rq->userId);
        QStringList roomLst;
        if (m_sql.SelectMySql(sql, 1, roomLst)) {
            qDebug() << "[DEBUG dealOfflineRq] rooms to destroy:" << roomLst.size() << "roomIds:" << roomLst;
            while (roomLst.size() > 0) {
                int roomId = roomLst.first().toInt();
                roomLst.removeFirst();
                qDebug() << "Offline cleanup: destroying room" << roomId << "of user" << rq->userId;

                // Broadcast PROT_ROOM_END_RS to all online room members before deletion
                {
                    snprintf(sql, sizeof(sql),
                             "SELECT userId FROM t_room_member WHERE roomId = %d", roomId);
                    QStringList memberLst;
                    if (m_sql.SelectMySql(sql, 1, memberLst)) {
                        PROT_ROOM_END_RS endRs;
                        endRs.result = 0;
                        endRs.roomId = roomId;
                        QMutexLocker locker(&m_mutex);
                        for (int i = 0; i < memberLst.size(); ++i) {
                            int uid = memberLst[i].toInt();
                            if (m_mapIdToSocket.contains(uid)) {
                                m_pMediator->sendData((char*)&endRs, sizeof(endRs), m_mapIdToSocket[uid]);
                            }
                        }
                    }
                }

                snprintf(sql, sizeof(sql),
                         "DELETE FROM t_room_member WHERE roomId = %d", roomId);
                m_sql.UpdateMySql(sql);

                snprintf(sql, sizeof(sql),
                         "DELETE FROM t_room WHERE roomId = %d", roomId);
                m_sql.UpdateMySql(sql);
            }
        }
    }
}

void CKernel::dealAddFriRq(char* data, int len, unsigned long from) {
    qDebug() << "========== 添加好友请求 ==========";
    qDebug() << "from socket:" << from;
    PROT_ADD_FRIEND_RQ* rq = (PROT_ADD_FRIEND_RQ*)data;

    qDebug() << "我的ID:" << rq->userId;
    qDebug() << "我的名字:" << rq->usename;
    qDebug() << "要添加的好友名字:" << rq->friendName;

    QByteArray safeFriendName = escapeString(rq->friendName);

    if (safeFriendName.isEmpty() || safeFriendName.size() > DEF_INFO_LEN - 1) {
        qDebug() << "好友名字长度无效";
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    char findFriendSql[256];
    snprintf(findFriendSql, sizeof(findFriendSql), "SELECT id FROM t_userinfo WHERE name = '%s'", safeFriendName.constData());
    qDebug() << "执行SQL:" << findFriendSql;
    QStringList findFriendLst;

    if (!m_sql.SelectMySql(findFriendSql, 1, findFriendLst) || findFriendLst.size() == 0) {
        qDebug() << "好友不存在!";
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    qDebug() << "找到好友ID:" << findFriendLst.first();

    int friendId = 0;
    bool ok;
    friendId = findFriendLst.first().toInt(&ok);
    if (!ok) {
        qDebug() << "转换好友ID失败";
        PROT_ADD_FRIEND_RS rs;
        rs.result = DEF_ADD_FRIEND_NOTEXIST;
        strcpy(rs.friendName, rq->friendName);
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    qDebug() << "好友ID:" << friendId;
    qDebug() << "检查好友是否在线...";

    {
        QMutexLocker locker(&m_mutex);
        qDebug() << "当前在线用户数:" << m_mapIdToSocket.size();
        if (m_mapIdToSocket.contains(friendId)) {
            qDebug() << "好友在线! 转发请求到好友 socket=" << m_mapIdToSocket[friendId];
            m_pMediator->sendData(data, len, m_mapIdToSocket[friendId]);
        } else {
            qDebug() << "好友不在线，发送离线响应";
            PROT_ADD_FRIEND_RS rs;
            rs.result = DEF_ADD_FRIEND_OFFLINE;
            strcpy(rs.friendName, rq->friendName);
            m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        }
    }
}

void CKernel::dealAddFriRs(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_ADD_FRIEND_RS* rs = (PROT_ADD_FRIEND_RS*)data;

    qDebug() << "添加好友结果:" << rs->result;
    qDebug() << "发送者ID:" << rs->userId;
    qDebug() << "好友ID:" << rs->friendId;

    if (DEF_ADD_FRIEND_ACCEPT == rs->result) {
        qDebug() << "好友同意了，插入数据库";
        char insertFriendSql[512];
        snprintf(insertFriendSql, sizeof(insertFriendSql),
                 "INSERT INTO t_friend (idA, idB) VALUES (%d, %d)", rs->friendId, rs->userId);
        qDebug() << "执行SQL1:" << insertFriendSql;
        if (!m_sql.UpdateMySql(insertFriendSql)) {
            qDebug() << "插入好友关系1失败";
        } else {
            qDebug() << "插入好友关系1成功";
        }

        snprintf(insertFriendSql, sizeof(insertFriendSql),
                 "INSERT INTO t_friend (idA, idB) VALUES (%d, %d)", rs->userId, rs->friendId);
        qDebug() << "执行SQL2:" << insertFriendSql;
        if (!m_sql.UpdateMySql(insertFriendSql)) {
            qDebug() << "插入好友关系2失败";
        } else {
            qDebug() << "插入好友关系2成功";
        }

        qDebug() << "获取好友信息:" << rs->friendId;
        getUserInfoAddFriendInfo(rs->friendId);
    }

    qDebug() << "转发响应给发送者: userId=" << rs->userId;
    {
        QMutexLocker locker(&m_mutex);
        if (m_mapIdToSocket.contains(rs->userId)) {
            qDebug() << "发送响应到 socket=" << m_mapIdToSocket[rs->userId];
            m_pMediator->sendData(data, len, m_mapIdToSocket[rs->userId]);
        } else {
            qDebug() << "发送者不在线，无法发送响应";
        }
    }
}

void CKernel::getInfoById(int id, char* info) {
    qDebug() << __func__;

    PROT_FRIEND_INFO* friendInfo = (PROT_FRIEND_INFO*)info;
    friendInfo->id = id;

    {
        QMutexLocker locker(&m_mutex);
        if (m_mapIdToSocket.contains(id)) {
            friendInfo->status = DEF_STATUS_ONLINE;
        } else {
            friendInfo->status = DEF_STATUS_OFFLINE;
        }
    }

    char sql[1024];
    snprintf(sql, sizeof(sql), "SELECT name, feeling, iconid FROM t_userinfo WHERE id = %d", id);
    QStringList lstStr;

    if (m_sql.SelectMySql(sql, 3, lstStr)) {
        if (lstStr.size() >= 3) {
            QString name = lstStr.first();
            lstStr.removeFirst();
            QString feeling = lstStr.first();
            lstStr.removeFirst();
            QString iconIdStr = lstStr.first();
            lstStr.removeFirst();

            int iconId = 8;
            bool ok;
            int tmpIconId = iconIdStr.toInt(&ok);
            if (ok) iconId = tmpIconId;

            QTextCodec* codec = QTextCodec::codecForName("GB2312");
            QByteArray nameBa = codec ? codec->fromUnicode(name) : name.toUtf8();
            QByteArray feelingBa = codec ? codec->fromUnicode(feeling) : feeling.toUtf8();
            strncpy(friendInfo->name, nameBa.constData(), DEF_INFO_LEN - 1);
            friendInfo->name[DEF_INFO_LEN - 1] = '\0';
            strncpy(friendInfo->feeling, feelingBa.constData(), DEF_INFO_LEN - 1);
            friendInfo->feeling[DEF_INFO_LEN - 1] = '\0';
            friendInfo->iconId = iconId;

            qDebug() << "获取用户信息成功: name=" << name << ", feeling=" << feeling << ", iconId=" << iconId;
        }
    }
}


// ============ Room Handlers ============

void CKernel::dealRoomCreateRq(char* data, int len, unsigned long from) {
    qDebug() << __func__ << "roomName=" << ((PROT_ROOM_CREATE_RQ*)data)->roomName << "userId=" << ((PROT_ROOM_CREATE_RQ*)data)->userId;
    PROT_ROOM_CREATE_RQ* rq = (PROT_ROOM_CREATE_RQ*)data;

    QByteArray safeName = escapeString(rq->roomName);
    QByteArray safePass = escapeString(rq->roomPass);

    if (safeName.isEmpty() || safeName.size() > DEF_INFO_LEN - 1) {
        PROT_ROOM_CREATE_RS rs;
        rs.result = DEF_ROOM_CREATE_FA;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    QByteArray passToUse = safePass.isEmpty() ? QByteArray("") : safePass;

    char sql[1024];
    // Begin transaction: t_room+t_room_member must be atomic
    m_sql.UpdateMySql("START TRANSACTION");

    // Generate unique 8-digit room number
    int roomNumber;
    char checkSql[256];
    bool unique;
    do {
        roomNumber = 10000000 + rand() % 90000000;
        snprintf(checkSql, sizeof(checkSql),
                 "SELECT COUNT(*) FROM t_room WHERE roomNumber = %d", roomNumber);
        QStringList checkLst2;
        unique = true;
        if (m_sql.SelectMySql(checkSql, 1, checkLst2) && checkLst2.size() > 0) {
            int cnt = checkLst2.first().toInt();
            if (cnt > 0) unique = false;
        }
    } while (!unique);

    snprintf(sql, sizeof(sql),
             "INSERT INTO t_room (roomName, roomPass, creatorId, roomNumber) VALUES ('%s', '%s', %d, %d)",
             safeName.constData(), passToUse.constData(), rq->userId, roomNumber);

    if (!m_sql.UpdateMySql(sql)) {
        m_sql.UpdateMySql("ROLLBACK");
        PROT_ROOM_CREATE_RS rs;
        rs.result = DEF_ROOM_CREATE_FA;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // Get the auto-increment roomId
    QStringList lstStr;
    if (!m_sql.SelectMySql("SELECT LAST_INSERT_ID()", 1, lstStr) || lstStr.size() == 0) {
        m_sql.UpdateMySql("ROLLBACK");
        PROT_ROOM_CREATE_RS rs;
        rs.result = DEF_ROOM_CREATE_FA;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    int roomId = 0;
    bool ok;
    roomId = lstStr.first().toInt(&ok);
    if (!ok) {
        m_sql.UpdateMySql("ROLLBACK");
        PROT_ROOM_CREATE_RS rs;
        rs.result = DEF_ROOM_CREATE_FA;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // Add creator to room members
    snprintf(sql, sizeof(sql),
             "INSERT INTO t_room_member (roomId, userId) VALUES (%d, %d)",
             roomId, rq->userId);
    if (!m_sql.UpdateMySql(sql)) {
        m_sql.UpdateMySql("ROLLBACK");
        PROT_ROOM_CREATE_RS rs;
        rs.result = DEF_ROOM_CREATE_FA;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    m_sql.UpdateMySql("COMMIT");

    PROT_ROOM_CREATE_RS rs;
    rs.result = DEF_ROOM_CREATE_SUC;
    rs.roomId = roomId;
    rs.roomNumber = roomNumber;
    strncpy(rs.roomName, rq->roomName, DEF_INFO_LEN - 1);
    rs.roomName[DEF_INFO_LEN - 1] = '\0';
    m_pMediator->sendData((char*)&rs, sizeof(rs), from);

    // Send member list to the creator
    {
        snprintf(sql, sizeof(sql),
                 "SELECT u.name, u.id FROM t_userinfo u "
                 "INNER JOIN t_room_member rm ON u.id = rm.userId "
                 "WHERE rm.roomId = %d ORDER BY rm.joinTime ASC",
                 roomId);
        QStringList crMemberLst;
        if (m_sql.SelectMySql(sql, 2, crMemberLst) && crMemberLst.size() > 0) {
            PROT_ROOM_MEMBER_LIST memberList;
            memberList.roomId = roomId;
            int totalMembers = crMemberLst.size() / 2;
            memberList.totalCount = totalMembers;
            memberList.memberCount = qMin(totalMembers, DEF_ROOM_MEMBER_MAX);
            int idx = 0;
            int memIdx = 0;
            while (idx + 1 < crMemberLst.size() && memIdx < DEF_ROOM_MEMBER_MAX) {
                QByteArray nameBytes = crMemberLst[idx].toUtf8();
                strncpy(memberList.members[memIdx].userName, nameBytes.constData(), DEF_INFO_LEN - 1);
                memberList.members[memIdx].userName[DEF_INFO_LEN - 1] = ' ';
                memberList.members[memIdx].userId = crMemberLst[idx + 1].toInt();
                memberList.members[memIdx].status = 0;
                idx += 2;
                memIdx++;
            }
            m_pMediator->sendData((char*)&memberList, sizeof(memberList), from);
        }
    }
}

void CKernel::dealRoomJoinRq(char* data, int len, unsigned long from) {
    qDebug() << __func__ << "roomId=" << ((PROT_ROOM_JOIN_RQ*)data)->roomId << "userId=" << ((PROT_ROOM_JOIN_RQ*)data)->userId;
    PROT_ROOM_JOIN_RQ* rq = (PROT_ROOM_JOIN_RQ*)data;
    char sql[1024];
    int realRoomId = 0;
    int roomNumber = 0;
    QString roomName;
    QString storedPass;
    bool roomFound = false;

    // Try lookup by roomId first
    snprintf(sql, sizeof(sql),
             "SELECT roomName, roomPass, roomNumber FROM t_room WHERE roomId = %d", rq->roomId);
    QStringList lstStr;
    if (m_sql.SelectMySql(sql, 3, lstStr) && lstStr.size() >= 3) {
        realRoomId = rq->roomId;
        roomName = lstStr.first();
        lstStr.removeFirst();
        storedPass = lstStr.first();
        lstStr.removeFirst();
        bool ok;
        roomNumber = lstStr.first().toInt(&ok);
        if (!ok) roomNumber = 0;
        roomFound = true;
    }

    // Fallback: try lookup by roomNumber
    if (!roomFound) {
        snprintf(sql, sizeof(sql),
                 "SELECT roomId, roomName, roomPass, roomNumber FROM t_room WHERE roomNumber = %d",
                 rq->roomId);
        QStringList lstStr2;
        if (m_sql.SelectMySql(sql, 4, lstStr2) && lstStr2.size() >= 4) {
            bool ok;
            realRoomId = lstStr2.first().toInt(&ok);
            lstStr2.removeFirst();
            roomName = lstStr2.first();
            lstStr2.removeFirst();
            storedPass = lstStr2.first();
            lstStr2.removeFirst();
            roomNumber = lstStr2.first().toInt(&ok);
            if (!ok) roomNumber = 0;
            roomFound = ok;
        }
    }

    if (!roomFound) {
        PROT_ROOM_JOIN_RS rs;
        rs.result = DEF_ROOM_JOIN_FA;
        rs.roomId = rq->roomId;
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        return;
    }

    // Check password if room has one
    if (!storedPass.isEmpty()) {
        QByteArray inputPass(rq->roomPass);
        if (inputPass != storedPass.toUtf8()) {
            PROT_ROOM_JOIN_RS rs;
            rs.result = DEF_ROOM_JOIN_PASSERR;
            rs.roomId = rq->roomId;
            m_pMediator->sendData((char*)&rs, sizeof(rs), from);
            return;
        }
    }

    // // Check if already a member - allow re-join after reconnect
    snprintf(sql, sizeof(sql),
             "SELECT userId FROM t_room_member WHERE roomId = %d AND userId = %d",
             realRoomId, rq->userId);
    QStringList checkLst;
    bool alreadyMember = (m_sql.SelectMySql(sql, 1, checkLst) && checkLst.size() > 0);

    if (!alreadyMember) {
        // Add to room
        snprintf(sql, sizeof(sql),
                 "INSERT INTO t_room_member (roomId, userId) VALUES (%d, %d)",
                 realRoomId, rq->userId);
        m_sql.UpdateMySql(sql);
    }

    // Send join success response
    PROT_ROOM_JOIN_RS rs;
    rs.result = DEF_ROOM_JOIN_SUC;
    rs.roomNumber = roomNumber;
    rs.roomId = realRoomId;
    // Check if this user is the room creator
    {
        snprintf(sql, sizeof(sql), "SELECT creatorId FROM t_room WHERE roomId = %d", realRoomId);
        QStringList clst;
        if (m_sql.SelectMySql(sql, 1, clst) && clst.size() > 0) {
            int cid = clst.first().toInt();
            rs.isCreator = (cid == rq->userId) ? 1 : 0;
        } else {
            rs.isCreator = 0;
        }
    }
    QByteArray nameBa = roomName.toUtf8();
    strncpy(rs.roomName, nameBa.constData(), DEF_INFO_LEN - 1);
    rs.roomName[DEF_INFO_LEN - 1] = '\0';
    m_pMediator->sendData((char*)&rs, sizeof(rs), from);

    // Send member list to the joining user
    snprintf(sql, sizeof(sql),
             "SELECT u.name, u.id FROM t_userinfo u "
             "INNER JOIN t_room_member rm ON u.id = rm.userId "
             "WHERE rm.roomId = %d ORDER BY rm.joinTime ASC",
             realRoomId);
    QStringList memberLst;
    if (m_sql.SelectMySql(sql, 2, memberLst) && memberLst.size() > 0) {
        PROT_ROOM_MEMBER_LIST memberList;
        memberList.roomId = realRoomId;
        int totalMembers = memberLst.size() / 2;
        memberList.totalCount = totalMembers;
        memberList.memberCount = qMin(totalMembers, DEF_ROOM_MEMBER_MAX);
        int idx = 0;
        int memIdx = 0;
        while (idx + 1 < memberLst.size() && memIdx < DEF_ROOM_MEMBER_MAX) {
            QByteArray nameBytes = memberLst[idx].toUtf8();
            strncpy(memberList.members[memIdx].userName, nameBytes.constData(), DEF_INFO_LEN - 1);
            memberList.members[memIdx].userName[DEF_INFO_LEN - 1] = '\0';
            memberList.members[memIdx].userId = memberLst[idx + 1].toInt();
            memberList.members[memIdx].status = 0;
            idx += 2;
            memIdx++;
        }
        m_pMediator->sendData((char*)&memberList, sizeof(memberList), from);
    }

    // Broadcast PROT_ROOM_INFO + MEMBER_LIST to other online room members
    {
        snprintf(sql, sizeof(sql),
                 "SELECT userId FROM t_room_member WHERE roomId = %d AND userId != %d",
                 realRoomId, rq->userId);
        QStringList others;
        if (m_sql.SelectMySql(sql, 1, others) && others.size() > 0) {
            // Get joiner name
            QString joinerName;
            snprintf(sql, sizeof(sql), "SELECT name FROM t_userinfo WHERE id = %d", rq->userId);
            QStringList jnLst;
            if (m_sql.SelectMySql(sql, 1, jnLst) && jnLst.size() > 0)
                joinerName = jnLst.first();
            if (joinerName.isEmpty()) joinerName = QString("User%1").arg(rq->userId);

            int total = others.size() + 1;  // +1 for joiner
            PROT_ROOM_INFO ri;
            ri.roomId = realRoomId;
            ri.memberCount = total;
            ri.eventType = 0;  // JOIN
            ri.userId = rq->userId;
            QByteArray rnb = roomName.toUtf8();
            strncpy(ri.roomName, rnb.constData(), DEF_INFO_LEN - 1);
            ri.roomName[DEF_INFO_LEN - 1] = '\0';
            QByteArray jnb = joinerName.toUtf8();
            strncpy(ri.userName, jnb.constData(), DEF_INFO_LEN - 1);
            ri.userName[DEF_INFO_LEN - 1] = '\0';

            QMutexLocker locker(&m_mutex);
            for (int i = 0; i < others.size(); ++i) {
                int uid = others[i].toInt();
                if (m_mapIdToSocket.contains(uid))
                    m_pMediator->sendData((char*)&ri, sizeof(ri), m_mapIdToSocket[uid]);
            }
        }

        // Also send full member list to others so their member panel updates
        snprintf(sql, sizeof(sql),
                 "SELECT u.name, u.id FROM t_userinfo u "
                 "INNER JOIN t_room_member rm ON u.id = rm.userId "
                 "WHERE rm.roomId = %d ORDER BY rm.joinTime ASC",
                 realRoomId);
        QStringList mlLst;
        if (m_sql.SelectMySql(sql, 2, mlLst) && mlLst.size() > 0) {
            PROT_ROOM_MEMBER_LIST memberList;
            memberList.roomId = realRoomId;
            int totalMembers = mlLst.size() / 2;
            memberList.totalCount = totalMembers;
            memberList.memberCount = qMin(totalMembers, DEF_ROOM_MEMBER_MAX);
            int idx = 0;
            int memIdx = 0;
            while (idx + 1 < mlLst.size() && memIdx < DEF_ROOM_MEMBER_MAX) {
                QByteArray nameBytes = mlLst[idx].toUtf8();
                strncpy(memberList.members[memIdx].userName, nameBytes.constData(), DEF_INFO_LEN - 1);
                memberList.members[memIdx].userName[DEF_INFO_LEN - 1] = '\0';
                memberList.members[memIdx].userId = mlLst[idx + 1].toInt();
                memberList.members[memIdx].status = 0;
                idx += 2;
                memIdx++;
            }
            QMutexLocker locker(&m_mutex);
            for (int i = 0; i < others.size(); ++i) {
                int uid = others[i].toInt();
                if (m_mapIdToSocket.contains(uid))
                    m_pMediator->sendData((char*)&memberList, sizeof(memberList), m_mapIdToSocket[uid]);
            }
        }
    }
}

void CKernel::dealRoomLeaveRq(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_ROOM_LEAVE_RQ* rq = (PROT_ROOM_LEAVE_RQ*)data;

    char sql[1024];

    // Get leaver name and room name BEFORE deleting
    QString leaverName;
    snprintf(sql, sizeof(sql), "SELECT name FROM t_userinfo WHERE id = %d", rq->userId);
    QStringList lnLst;
    if (m_sql.SelectMySql(sql, 1, lnLst) && lnLst.size() > 0)
        leaverName = lnLst.first();
    if (leaverName.isEmpty()) leaverName = QString("User%1").arg(rq->userId);

    QString lvRoomName;
    snprintf(sql, sizeof(sql), "SELECT roomName FROM t_room WHERE roomId = %d", rq->roomId);
    QStringList rnLst;
    if (m_sql.SelectMySql(sql, 1, rnLst) && rnLst.size() > 0)
        lvRoomName = rnLst.first();
    if (lvRoomName.isEmpty()) lvRoomName = QString("Room%1").arg(rq->roomId);

    // Remove this member from the room
    snprintf(sql, sizeof(sql),
             "DELETE FROM t_room_member WHERE roomId = %d AND userId = %d",
             rq->roomId, rq->userId);
    m_sql.UpdateMySql(sql);

    // Check if room is now empty — destroy if last member left
    {
        snprintf(sql, sizeof(sql),
                 "SELECT COUNT(*) FROM t_room_member WHERE roomId = %d", rq->roomId);
        QStringList cntLst;
        if (m_sql.SelectMySql(sql, 1, cntLst) && cntLst.size() > 0 && cntLst.first().toInt() == 0) {
            qDebug() << "Room" << rq->roomId << "is empty, auto-destroying (last member left)";
            m_sql.UpdateMySql("START TRANSACTION");
            snprintf(sql, sizeof(sql),
                     "DELETE FROM t_room WHERE roomId = %d", rq->roomId);
            if (!m_sql.UpdateMySql(sql)) {
                m_sql.UpdateMySql("ROLLBACK");
            } else {
                m_sql.UpdateMySql("COMMIT");
                // Send END_RS instead of LEAVE_RS so client shows "会议已结束"
                PROT_ROOM_END_RS endRs;
                endRs.result = 0;
                endRs.roomId = rq->roomId;
                m_pMediator->sendData((char*)&endRs, sizeof(endRs), from);
                return;
            }
        }
    }

    // Broadcast PROT_ROOM_INFO + MEMBER_LIST to remaining online members
    {
        snprintf(sql, sizeof(sql),
                 "SELECT userId FROM t_room_member WHERE roomId = %d", rq->roomId);
        QStringList remaining;
        if (m_sql.SelectMySql(sql, 1, remaining)) {
            int total = remaining.size();
            PROT_ROOM_INFO ri;
            ri.roomId = rq->roomId;
            ri.memberCount = total;
            ri.eventType = 1;  // LEAVE
            ri.userId = rq->userId;
            QByteArray rnb = lvRoomName.toUtf8();
            strncpy(ri.roomName, rnb.constData(), DEF_INFO_LEN - 1);
            ri.roomName[DEF_INFO_LEN - 1] = '\0';
            QByteArray lnb = leaverName.toUtf8();
            strncpy(ri.userName, lnb.constData(), DEF_INFO_LEN - 1);
            ri.userName[DEF_INFO_LEN - 1] = '\0';

            QMutexLocker locker(&m_mutex);
            for (int i = 0; i < remaining.size(); ++i) {
                int uid = remaining[i].toInt();
                if (m_mapIdToSocket.contains(uid))
                    m_pMediator->sendData((char*)&ri, sizeof(ri), m_mapIdToSocket[uid]);
            }
        }

        // Also send full member list to remaining members so their panel updates
        snprintf(sql, sizeof(sql),
                 "SELECT u.name, u.id FROM t_userinfo u "
                 "INNER JOIN t_room_member rm ON u.id = rm.userId "
                 "WHERE rm.roomId = %d ORDER BY rm.joinTime ASC",
                 rq->roomId);
        QStringList mlLst;
        if (m_sql.SelectMySql(sql, 2, mlLst) && mlLst.size() > 0) {
            PROT_ROOM_MEMBER_LIST memberList;
            memberList.roomId = rq->roomId;
            int totalMembers = mlLst.size() / 2;
            memberList.totalCount = totalMembers;
            memberList.memberCount = qMin(totalMembers, DEF_ROOM_MEMBER_MAX);
            int idx = 0;
            int memIdx = 0;
            while (idx + 1 < mlLst.size() && memIdx < DEF_ROOM_MEMBER_MAX) {
                QByteArray nameBytes = mlLst[idx].toUtf8();
                strncpy(memberList.members[memIdx].userName, nameBytes.constData(), DEF_INFO_LEN - 1);
                memberList.members[memIdx].userName[DEF_INFO_LEN - 1] = '\0';
                memberList.members[memIdx].userId = mlLst[idx + 1].toInt();
                memberList.members[memIdx].status = 0;
                idx += 2;
                memIdx++;
            }
            QMutexLocker locker(&m_mutex);
            for (int i = 0; i < remaining.size(); ++i) {
                int uid = remaining[i].toInt();
                if (m_mapIdToSocket.contains(uid))
                    m_pMediator->sendData((char*)&memberList, sizeof(memberList), m_mapIdToSocket[uid]);
            }
        }
    }

    PROT_ROOM_LEAVE_RS rs;
    rs.result = DEF_ROOM_LEAVE_SUC;
    rs.roomId = rq->roomId;
    m_pMediator->sendData((char*)&rs, sizeof(rs), from);
}

void CKernel::dealRoomEndRq(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_ROOM_END_RQ* rq = (PROT_ROOM_END_RQ*)data;

    char sql[1024];

    // Verify the sender is the room creator
    snprintf(sql, sizeof(sql),
             "SELECT creatorId FROM t_room WHERE roomId = %d", rq->roomId);
    QStringList lstStr;
    bool isCreator = false;
    if (m_sql.SelectMySql(sql, 1, lstStr) && lstStr.size() > 0) {
        int creatorId = lstStr.first().toInt();
        isCreator = (creatorId == rq->userId);
    }

    PROT_ROOM_END_RS rs;
    rs.result = isCreator ? 0 : 1;
    rs.roomId = rq->roomId;

    if (isCreator) {
        // Query all room members BEFORE deleting the room
        snprintf(sql, sizeof(sql),
                 "SELECT userId FROM t_room_member WHERE roomId = %d", rq->roomId);
        QStringList memberLst;
        m_sql.SelectMySql(sql, 1, memberLst);  // save member list before deletion

        // Destroy the room FIRST (so clients refreshing their list won't see it)
        m_sql.UpdateMySql("START TRANSACTION");
        snprintf(sql, sizeof(sql),
                 "DELETE FROM t_room_member WHERE roomId = %d", rq->roomId);
        if (!m_sql.UpdateMySql(sql)) {
            m_sql.UpdateMySql("ROLLBACK");
        } else {
            snprintf(sql, sizeof(sql),
                     "DELETE FROM t_room WHERE roomId = %d", rq->roomId);
            if (!m_sql.UpdateMySql(sql)) {
                m_sql.UpdateMySql("ROLLBACK");
            } else {
                m_sql.UpdateMySql("COMMIT");
            }
        }

        // Broadcast END_RS to all online members AFTER deletion
        {
            QMutexLocker locker(&m_mutex);
            for (int i = 0; i < memberLst.size(); ++i) {
                int uid = memberLst[i].toInt();
                if (uid == rq->userId) continue;  // skip requester, sent separately below
                if (m_mapIdToSocket.contains(uid)) {
                    unsigned long sock = m_mapIdToSocket[uid];
                    m_pMediator->sendData((char*)&rs, sizeof(rs), sock);
                }
            }
        }
        // Send to requester (ensures delivery even if not in online map)
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
    } else {
        // Non-creator: send failure only to requester
        m_pMediator->sendData((char*)&rs, sizeof(rs), from);
    }
}

void CKernel::dealRoomChatRq(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_ROOM_CHAT_RQ* rq = (PROT_ROOM_CHAT_RQ*)data;

    char sql[1024];

    // Check sender is a room member
    snprintf(sql, sizeof(sql),
             "SELECT 1 FROM t_room_member WHERE roomId = %d AND userId = %d",
             rq->roomId, rq->userId);
    QStringList memberCheckLst;
    if (!m_sql.SelectMySql(sql, 1, memberCheckLst) || memberCheckLst.size() == 0) {
        return;  // not a member, silently ignore
    }

    // Get sender name
    snprintf(sql, sizeof(sql), "SELECT name FROM t_userinfo WHERE id = %d", rq->userId);
    QStringList nameLst;
    char senderName[DEF_INFO_LEN];
    memset(senderName, 0, DEF_INFO_LEN);
    if (m_sql.SelectMySql(sql, 1, nameLst) && nameLst.size() > 0) {
        QByteArray nameBa = nameLst.first().toUtf8();
        strncpy(senderName, nameBa.constData(), DEF_INFO_LEN - 1);
        senderName[DEF_INFO_LEN - 1] = '\0';
    }

    // Build reply packet
    PROT_ROOM_CHAT_RS rs;
    rs.userId = rq->userId;
    rs.roomId = rq->roomId;
    rs.result = DEF_ROOM_CHAT_SUC;
    strncpy(rs.senderName, senderName, DEF_INFO_LEN - 1);
    rs.senderName[DEF_INFO_LEN - 1] = '\0';
    strncpy(rs.content, rq->content, DEF_CHATMSG_LEN - 1);
    rs.content[DEF_CHATMSG_LEN - 1] = '\0';

    // Get all room members and send to each online member
    snprintf(sql, sizeof(sql), "SELECT userId FROM t_room_member WHERE roomId = %d", rq->roomId);
    QStringList memberLst;
    if (m_sql.SelectMySql(sql, 1, memberLst)) {
        // Collect target sockets under lock, then send outside lock
        QList<int> targets;
        {
            QMutexLocker locker(&m_mutex);
            while (memberLst.size() > 0) {
                bool ok;
                int memberId = memberLst.first().toInt(&ok);
                memberLst.removeFirst();
                if (!ok) continue;

                if (m_mapIdToSocket.contains(memberId)) {
                    targets.append(m_mapIdToSocket[memberId]);
                }
            }
        }
        for (int i = 0; i < targets.size(); ++i) {
            m_pMediator->sendData((char*)&rs, sizeof(rs), (unsigned long)targets[i]);
        }
    }
}

void CKernel::dealAudioFrame(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_AUDIO_FRAME* frame = (PROT_AUDIO_FRAME*)data;

    char sql[1024];

    // Get all room members and forward to each online member (except sender)
    snprintf(sql, sizeof(sql), "SELECT userId FROM t_room_member WHERE roomId = %d", frame->roomId);
    QStringList memberLst;
    if (m_sql.SelectMySql(sql, 1, memberLst)) {
        QList<int> targets;
        {
            QMutexLocker locker(&m_mutex);
            while (memberLst.size() > 0) {
                bool ok;
                int memberId = memberLst.first().toInt(&ok);
                memberLst.removeFirst();
                if (!ok) continue;
                if (memberId == frame->userId) continue;  // skip sender
                if (m_mapIdToSocket.contains(memberId)) {
                    targets.append(m_mapIdToSocket[memberId]);
                }
            }
        }
        for (int i = 0; i < targets.size(); ++i) {
            m_pMediator->sendData((char*)frame, sizeof(PROT_AUDIO_FRAME), (unsigned long)targets[i]);
        }
    }
}

void CKernel::dealVideoFrame(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_VIDEO_FRAME* frame = (PROT_VIDEO_FRAME*)data;

    char sql[1024];

    // Get all room members and forward to each online member (except sender)
    snprintf(sql, sizeof(sql), "SELECT userId FROM t_room_member WHERE roomId = %d", frame->roomId);
    QStringList memberLst;
    if (m_sql.SelectMySql(sql, 1, memberLst)) {
        QList<int> targets;
        {
            QMutexLocker locker(&m_mutex);
            while (memberLst.size() > 0) {
                bool ok;
                int memberId = memberLst.first().toInt(&ok);
                memberLst.removeFirst();
                if (!ok) continue;
                if (memberId == frame->userId) continue;  // skip sender
                if (m_mapIdToSocket.contains(memberId)) {
                    targets.append(m_mapIdToSocket[memberId]);
                }
            }
        }
        for (int i = 0; i < targets.size(); ++i) {
            m_pMediator->sendData((char*)frame, sizeof(PROT_VIDEO_FRAME), (unsigned long)targets[i]);
        }
    }
}

void CKernel::dealRoomListRq(char* data, int len, unsigned long from) {
    qDebug() << __func__;
    PROT_ROOM_LIST_RQ* rq = (PROT_ROOM_LIST_RQ*)data;

    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT r.roomId, r.roomName, r.roomNumber, "
             "(SELECT COUNT(*) FROM t_room_member WHERE roomId=r.roomId) as cnt, "
             "(CASE WHEN m.userId IS NOT NULL THEN 1 ELSE 0 END) AS joined, "
             "r.creatorId "
             "FROM t_room r LEFT JOIN t_room_member m ON r.roomId=m.roomId AND m.userId=%d",
             rq->userId);
    QStringList lstStr;
    if (m_sql.SelectMySql(sql, 6, lstStr)) {
        while (lstStr.size() >= 6) {
            QString roomIdStr = lstStr.first();
            lstStr.removeFirst();
            QString roomNameStr = lstStr.first();
            lstStr.removeFirst();
            QString roomNumberStr = lstStr.first();
            lstStr.removeFirst();
            QString cntStr = lstStr.first();
            lstStr.removeFirst();
            QString joinedStr = lstStr.first();
            lstStr.removeFirst();

            bool ok;
            int roomId = roomIdStr.toInt(&ok);
            if (!ok) continue;
            int roomNumberInt = roomNumberStr.toInt(&ok);
            if (!ok) roomNumberInt = 0;
            int cnt = cntStr.toInt(&ok);
            if (!ok) cnt = 0;
            int joined = joinedStr.toInt(&ok);
            if (!ok) joined = 0;

            QString creatorIdStr = lstStr.first();
            lstStr.removeFirst();
            int creatorId = creatorIdStr.toInt(&ok);
            if (!ok) creatorId = 0;

            PROT_ROOM_LIST_RS rs;
            rs.roomId = roomId;
            rs.memberCount = cnt;
            rs.joined = joined;
            rs.isCreator = (creatorId == rq->userId) ? 1 : 0;
            rs.roomNumber = roomNumberInt;
            QByteArray nameBa = roomNameStr.toUtf8();
            strncpy(rs.roomName, nameBa.constData(), DEF_INFO_LEN - 1);
            rs.roomName[DEF_INFO_LEN - 1] = '\0';
            m_pMediator->sendData((char*)&rs, sizeof(rs), from);
        }
    }
}


// ========== Audio/Video Forwarding ==========
