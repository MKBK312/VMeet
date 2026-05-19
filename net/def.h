#pragma once
#include<string.h>
#include <string>
//加载库版本号
#define _DEF_VERSION_LOW (2)
#define _DEF_VERSION_HIGH (2)
//udp端口号
#define  _DEF_UDP_PORT (12345)
//tcp端口号
#define _DEF_TCP_PORT (6789)
//tcp监听队列的最大长度 100
#define _DEF_TCP_LISTEN_MAX (100)
//----------------------------------------------
//基本信息的长度
#define DEF_INFO_LEN        (20)

//-----------------------------
#define DEF_PROTOCOL_BASE   1000
//RQ:请求  RS:回应
//注册:
#define DEF_REGISTER_RQ     (DEF_PROTOCOL_BASE+0)
#define DEF_REGISTER_RS     (DEF_PROTOCOL_BASE+1)

//登录:
#define DEF_LOGIN_RQ        (DEF_PROTOCOL_BASE+2)
#define DEF_LOGIN_RS        (DEF_PROTOCOL_BASE+3)

//添加朋友
#define DEF_ADD_FRIEND_RQ   (DEF_PROTOCOL_BASE+7)
#define DEF_ADD_FRIEND_RS   (DEF_PROTOCOL_BASE+8)

//聊天请求
#define DEF_CHATMSG_RQ      (DEF_PROTOCOL_BASE+5)
#define DEF_CHATMSG_RS      (DEF_PROTOCOL_BASE+6)


//下线请求
#define DEF_FRIEND_OFFLINE  (DEF_PROTOCOL_BASE+9)

//获取朋友信息
#define DEF_FRIEND_INFO  (DEF_PROTOCOL_BASE+10)

// Room protocols
#define DEF_ROOM_CREATE_RQ   (DEF_PROTOCOL_BASE+11)
#define DEF_ROOM_CREATE_RS   (DEF_PROTOCOL_BASE+12)
#define DEF_ROOM_JOIN_RQ     (DEF_PROTOCOL_BASE+13)
#define DEF_ROOM_JOIN_RS     (DEF_PROTOCOL_BASE+14)
#define DEF_ROOM_LEAVE_RQ    (DEF_PROTOCOL_BASE+15)
#define DEF_ROOM_LEAVE_RS    (DEF_PROTOCOL_BASE+16)
#define DEF_ROOM_CHAT_RQ     (DEF_PROTOCOL_BASE+17)
#define DEF_ROOM_CHAT_RS     (DEF_PROTOCOL_BASE+18)
#define DEF_ROOM_LIST_RQ     (DEF_PROTOCOL_BASE+19)
#define DEF_ROOM_LIST_RS     (DEF_PROTOCOL_BASE+20)
#define DEF_ROOM_INFO        (DEF_PROTOCOL_BASE+21)
#define DEF_ROOM_END_RQ     (DEF_PROTOCOL_BASE+22)
#define DEF_ROOM_END_RS     (DEF_PROTOCOL_BASE+23)
#define DEF_ROOM_MEMBER_LIST  (DEF_PROTOCOL_BASE+24)  // 1024
#define DEF_AUDIO_FRAME       (DEF_PROTOCOL_BASE+25)  // 1025
#define DEF_VIDEO_FRAME       (DEF_PROTOCOL_BASE+26)  // 1026
#define DEF_ROOM_MEMBER_MAX  (50)

// Room operation results
#define DEF_ROOM_CREATE_SUC       (0)
#define DEF_ROOM_CREATE_FA        (1)
#define DEF_ROOM_JOIN_SUC         (0)
#define DEF_ROOM_JOIN_FA          (1)
#define DEF_ROOM_JOIN_PASSERR     (2)
#define DEF_ROOM_LEAVE_SUC        (0)
#define DEF_ROOM_CHAT_SUC         (0)
#define DEF_ROOM_CHAT_FA          (1)

//注册结果
#define DEF_REGISTER_SUC    (0)
#define DEF_REGISTER_NAME_EXIST     (1)
#define DEF_REGISTER_TEL_EXIST     (2)

//登录结果
#define DEF_LOGIN_SUC       (0)
#define DEF_LOGIN_USERNOTEXIST  (1)
#define DEF_LOGIN_PASSERR  (2)

//添加好友回复的结果（4种）
#define DEF_ADD_FRIEND_ACCEPT   (0)
#define DEF_ADD_FRIEND_OFFLINE  (1)
#define DEF_ADD_FRIEND_REJECT   (2)
#define DEF_ADD_FRIEND_NOTEXIST (3)

//聊天内容的长度
#define DEF_CHATMSG_LEN     (1024*8) //8k
//结构体宏的个数
#define  DEF_PROTOCOL_COUNT 28

//发送聊天信息的结构
#define DEF_SEND_CHAT_SUC  (0)
#define DEF_SEND_CHAT_FA   (1)

//用户状态
#define DEF_STATUS_ONLINE   (0)
#define DEF_STATUS_OFFLINE   (1)


typedef int packageType;

//注册请求:结果（成功，电话号已被注册，昵称已被注册）
//请求结构体
struct PROT_REGISTER_RQ {
    packageType packtype; //协议类型
    char name[DEF_INFO_LEN];
    char tel[DEF_INFO_LEN];
    char pass[DEF_INFO_LEN];
    PROT_REGISTER_RQ() :packtype(DEF_REGISTER_RQ),name{ 0 }, tel{ 0 }, pass{ 0 } {}
};


//注册回复
//注册请求的回复
struct PROT_REGISTER_RS {
    packageType packtype; //协议类型
    int result; //注册的结果（成功，失败）
    PROT_REGISTER_RS() :packtype(DEF_REGISTER_RS), result(0) {}
    PROT_REGISTER_RS(int _result) :packtype(DEF_REGISTER_RS), result(_result) {}
};



//登录请求
struct PROT_LOGIN_RQ {
    packageType packtype; //协议类型
    char tel[DEF_INFO_LEN];
    char pass[DEF_INFO_LEN];
    PROT_LOGIN_RQ() :packtype(DEF_LOGIN_RQ),tel{ 0 }, pass{ 0 } {}
};

//登录回复
struct PROT_LOGIN_RS {
    packageType packtype; //协议类型
    int userId;
    int result;   //成功，失败（用户不存在，密码错误).
    PROT_LOGIN_RS() :packtype(DEF_LOGIN_RS), userId(0), result(DEF_LOGIN_PASSERR) {}
};


//添加好友请求:好友昵称，自己的id，自己的昵称
struct PROT_ADD_FRIEND_RQ {
    packageType packtype; //协议类型
    int userId;//我自己的ID
    char username[DEF_INFO_LEN];//我的昵称
    char friendName[DEF_INFO_LEN];//要添加的朋友的昵称

    PROT_ADD_FRIEND_RQ() :packtype(DEF_ADD_FRIEND_RQ), userId(0) {
        memset(username, 0, DEF_INFO_LEN);
        memset(friendName, 0, DEF_INFO_LEN);
    }
};


//添加好友回复:结果（添加成功，好友不存在，好友已拒绝，好友不在线），自己的id，自己的昵称，好友的id，好友的昵称
struct PROT_ADD_FRIEND_RS {
    packageType packtype; //协议类型
    int result;//4种
    int userId;//我
    int friendId;//回复给对方的ID
    char userName[DEF_INFO_LEN];
    char friendName[DEF_INFO_LEN];

    PROT_ADD_FRIEND_RS() :
        packtype(DEF_ADD_FRIEND_RS),
        result(DEF_ADD_FRIEND_NOTEXIST), userId(0), friendId(0) {
        memset(userName, 0, DEF_INFO_LEN);
        memset(friendName, 0, DEF_INFO_LEN);
    }

};


//聊天请求:聊天内容，自己的id，好友的id
struct PROT_CHAT_RQ {
    packageType packtype; //协议类型
    int userId;//我的ID
    int friendId;//我的某个朋友
    char content[DEF_CHATMSG_LEN];
    PROT_CHAT_RQ() : packtype(DEF_CHATMSG_RQ),userId(0), friendId(0) {
        memset(content, 0, DEF_CHATMSG_LEN);
    }
};

//聊天回复
struct PROT_CHAT_INFO_RS {
    packageType packtype; //协议类型
    int friendId;
    int result;
    PROT_CHAT_INFO_RS() :packtype(DEF_CHATMSG_RS),friendId(0),
        result(DEF_SEND_CHAT_FA) {}
};

//下线请求:自己的id
struct PROT_FRIEND_OFFLINE {
    packageType packtype; //协议类型
    int userId;
    PROT_FRIEND_OFFLINE() :packtype(DEF_FRIEND_OFFLINE),userId(0)
    {}
};

//获取朋友信息
struct PROT_FRIEND_INFO
{
    packageType packtype; //协议类型
    int id;
    int iconId;
    int status;
    char name[DEF_INFO_LEN];
    char feeling[DEF_INFO_LEN];
    PROT_FRIEND_INFO() :packtype(DEF_FRIEND_INFO), id(0), iconId(0), status(DEF_STATUS_OFFLINE) {
        memset(name, 0, DEF_INFO_LEN);
        memset(feeling, 0, DEF_INFO_LEN);
    }

};

// Room create request
struct PROT_ROOM_CREATE_RQ {
    packageType packtype;
    int userId;
    char roomName[DEF_INFO_LEN];
    char roomPass[DEF_INFO_LEN];
    PROT_ROOM_CREATE_RQ() :packtype(DEF_ROOM_CREATE_RQ), userId(0) {
        memset(roomName, 0, DEF_INFO_LEN);
        memset(roomPass, 0, DEF_INFO_LEN);
    }
};

// Room create response
struct PROT_ROOM_CREATE_RS {
    packageType packtype;
    int result;
    int roomId;
    int roomNumber;  // 8-digit room number for joining
    char roomName[DEF_INFO_LEN];
    PROT_ROOM_CREATE_RS() :packtype(DEF_ROOM_CREATE_RS), result(DEF_ROOM_CREATE_FA), roomId(0), roomNumber(0) {
        memset(roomName, 0, DEF_INFO_LEN);
    }
};

// Room join request
struct PROT_ROOM_JOIN_RQ {
    packageType packtype;
    int userId;
    int roomId;
    char roomPass[DEF_INFO_LEN];
    PROT_ROOM_JOIN_RQ() :packtype(DEF_ROOM_JOIN_RQ), userId(0), roomId(0) {
        memset(roomPass, 0, DEF_INFO_LEN);
    }
};

// Room join response
struct PROT_ROOM_JOIN_RS {
    packageType packtype;
    int result;
    int roomId;
    int roomNumber;  // 8-digit room number
    int isCreator;   // 1=此用户是房间创建者
    char roomName[DEF_INFO_LEN];
    PROT_ROOM_JOIN_RS() :packtype(DEF_ROOM_JOIN_RS), result(DEF_ROOM_JOIN_FA), roomId(0), roomNumber(0), isCreator(0) {
        memset(roomName, 0, DEF_INFO_LEN);
    }
};

// Room leave request
struct PROT_ROOM_LEAVE_RQ {
    packageType packtype;
    int userId;
    int roomId;
    PROT_ROOM_LEAVE_RQ() :packtype(DEF_ROOM_LEAVE_RQ), userId(0), roomId(0) {}
};

// Room leave response
struct PROT_ROOM_LEAVE_RS {
    packageType packtype;
    int result;
    int roomId;
    PROT_ROOM_LEAVE_RS() :packtype(DEF_ROOM_LEAVE_RS), result(0), roomId(0) {}
};

// Room chat request
struct PROT_ROOM_CHAT_RQ {
    packageType packtype;
    int userId;
    int roomId;
    char content[DEF_CHATMSG_LEN];
    PROT_ROOM_CHAT_RQ() :packtype(DEF_ROOM_CHAT_RQ), userId(0), roomId(0) {
        memset(content, 0, DEF_CHATMSG_LEN);
    }
};

// Room chat response
struct PROT_ROOM_CHAT_RS {
    packageType packtype;
    int userId;
    int roomId;
    int result;
    char senderName[DEF_INFO_LEN];
    char content[DEF_CHATMSG_LEN];
    PROT_ROOM_CHAT_RS() :packtype(DEF_ROOM_CHAT_RS), userId(0), roomId(0), result(DEF_ROOM_CHAT_FA) {
        memset(senderName, 0, DEF_INFO_LEN);
        memset(content, 0, DEF_CHATMSG_LEN);
    }
};

// Room list request
struct PROT_ROOM_LIST_RQ {
    packageType packtype;
    int userId;
    PROT_ROOM_LIST_RQ() :packtype(DEF_ROOM_LIST_RQ), userId(0) {}
};

// Room list response
struct PROT_ROOM_LIST_RS {
    packageType packtype;
    int roomId;
    int memberCount;
    int joined;  // 1=已加入, 0=未加入
    int isCreator;  // 1=此用户是房间创建者
    int roomNumber;  // 8-digit room number
    char roomName[DEF_INFO_LEN];
    PROT_ROOM_LIST_RS() :packtype(DEF_ROOM_LIST_RS), roomId(0), memberCount(0), joined(0), isCreator(0), roomNumber(0) {
        memset(roomName, 0, DEF_INFO_LEN);
    }
};

// Room end request
struct PROT_ROOM_END_RQ {
    packageType packtype;
    int userId;
    int roomId;
    PROT_ROOM_END_RQ() :packtype(DEF_ROOM_END_RQ), userId(0), roomId(0) {}
};

// Room end response
struct PROT_ROOM_END_RS {
    packageType packtype;
    int result;
    int roomId;
    PROT_ROOM_END_RS() :packtype(DEF_ROOM_END_RS), result(0), roomId(0) {}
};

// Room member list (server sends to new joiner)
struct PROT_ROOM_MEMBER_ITEM {
    int userId;
    char userName[DEF_INFO_LEN];
    int status;  // 0=in-room
};

struct PROT_ROOM_MEMBER_LIST {
    packageType packtype;
    int roomId;
    int memberCount;
    int totalCount;
    PROT_ROOM_MEMBER_ITEM members[DEF_ROOM_MEMBER_MAX];
    PROT_ROOM_MEMBER_LIST() :packtype(DEF_ROOM_MEMBER_LIST), roomId(0),
        memberCount(0), totalCount(0) {
        memset(members, 0, sizeof(members));
    }
};

// Room info (broadcast on member join/leave)
struct PROT_ROOM_INFO {
    packageType packtype;
    int roomId;
    char roomName[DEF_INFO_LEN];
    int memberCount;
    int eventType;              // 0=JOIN, 1=LEAVE
    int userId;                 // who joined/left
    char userName[DEF_INFO_LEN];// who joined/left
    PROT_ROOM_INFO() :packtype(DEF_ROOM_INFO), roomId(0), memberCount(0),
                      eventType(0), userId(0) {
        memset(roomName, 0, DEF_INFO_LEN);
        memset(userName, 0, DEF_INFO_LEN);
    }
};

// Audio frame (Speex NB encoded, ~76 bytes per 40ms frame)
#define DEF_AUDIO_FRAME_SIZE 800  // raw PCM 640 bytes + margin

struct PROT_AUDIO_FRAME {
    packageType packtype;
    int userId;
    int roomId;
    int frameSize;  // actual encoded bytes
    char data[DEF_AUDIO_FRAME_SIZE];
    PROT_AUDIO_FRAME() :packtype(DEF_AUDIO_FRAME), userId(0), roomId(0), frameSize(0) {
        memset(data, 0, DEF_AUDIO_FRAME_SIZE);
    }
};

// Video frame (JPEG encoded, ~10-20KB per frame at 320x240)
#define DEF_VIDEO_FRAME_SIZE 65536  // enough for 960x540 JPEG screen share

struct PROT_VIDEO_FRAME {
    packageType packtype;
    int userId;
    int roomId;
    int frameSize;  // actual JPEG bytes
    char data[DEF_VIDEO_FRAME_SIZE];
    PROT_VIDEO_FRAME() :packtype(DEF_VIDEO_FRAME), userId(0), roomId(0), frameSize(0) {
        memset(data, 0, DEF_VIDEO_FRAME_SIZE);
    }
};
