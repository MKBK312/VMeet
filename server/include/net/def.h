#pragma once

#include <cstdint>
#include <string.h>
//-----------------------------
#define DEF_PROTOCOL_BASE   1000

//注册:
#define DEF_REGISTER_RQ     (DEF_PROTOCOL_BASE+0)
#define DEF_REGISTER_RS     (DEF_PROTOCOL_BASE+1)

//登录:
#define DEF_LOGIN_RQ        (DEF_PROTOCOL_BASE+2)
#define DEF_LOGIN_RS        (DEF_PROTOCOL_BASE+3)

//添加好友:
#define DEF_ADD_FRIEND_RQ   (DEF_PROTOCOL_BASE+7)
#define DEF_ADD_FRIEND_RS   (DEF_PROTOCOL_BASE+8)

//聊天消息:
#define DEF_CHATMSG_RQ      (DEF_PROTOCOL_BASE+5)
#define DEF_CHATMSG_RS      (DEF_PROTOCOL_BASE+6)

//好友下线:
#define DEF_FRIEND_OFFLINE  (DEF_PROTOCOL_BASE+9)

//获取好友信息
#define DEF_FRIEND_INFO     (DEF_PROTOCOL_BASE+10)

//-----------------------------
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
#define DEF_ROOM_END_RQ     (DEF_PROTOCOL_BASE+22)  // 1022
#define DEF_ROOM_END_RS     (DEF_PROTOCOL_BASE+23)  // 1023
#define DEF_ROOM_MEMBER_LIST  (DEF_PROTOCOL_BASE+24)  // 1024 (reserved)
#define DEF_ROOM_EVENT_JOIN   (0)
#define DEF_ROOM_EVENT_LEAVE  (1)

// Audio/Video frame forwarding
#define DEF_AUDIO_FRAME       (DEF_PROTOCOL_BASE+25)
#define DEF_VIDEO_FRAME       (DEF_PROTOCOL_BASE+26)
//-----------------------------

//-----------------------------
//注册结果
#define DEF_REGISTER_SUC    (0)
#define DEF_REGISTER_NAME_EXIST     (1)
#define DEF_REGISTER_TEL_EXIST     (2)

//登录结果
#define DEF_LOGIN_SUC       (0)
#define DEF_LOGIN_USERNOTEXIST  (1)
#define DEF_LOGIN_PASSERR    (2)

//添加好友返回结果的4种:
#define DEF_ADD_FRIEND_ACCEPT   (0)
#define DEF_ADD_FRIEND_OFFLINE  (1)
#define DEF_ADD_FRIEND_REJECT   (2)
#define DEF_ADD_FRIEND_NOTEXIST (3)

//发送消息的长度
#define DEF_CHATMSG_LEN     (1024*8) //8k
//结构体的个数
#define DEF_PROTOCOL_COUNT 28

//发送消息的结果
#define DEF_SEND_CHAT_SUC  (0)
#define DEF_SEND_CHAT_FA   (1)

// Room operation results
#define DEF_ROOM_CREATE_SUC       (0)
#define DEF_ROOM_CREATE_FA        (1)
#define DEF_ROOM_JOIN_SUC         (0)
#define DEF_ROOM_JOIN_FA          (1)
#define DEF_ROOM_JOIN_PASSERR     (2)
#define DEF_ROOM_LEAVE_SUC        (0)
#define DEF_ROOM_CHAT_SUC         (0)
#define DEF_ROOM_CHAT_FA          (1)

//用户状态
#define DEF_STATUS_ONLINE   (0)
#define DEF_STATUS_OFFLINE  (1)

//信息长度
#define DEF_INFO_LEN        (20)

typedef int32_t packageType;

//注册请求:登录名称成功，电话已被注册，昵称已被注册
struct PROT_REGISTER_RQ {
    packageType packtype;
    char name[DEF_INFO_LEN];
    char tel[DEF_INFO_LEN];
    char pass[DEF_INFO_LEN];
    PROT_REGISTER_RQ() :packtype(DEF_REGISTER_RQ), name{ 0 }, tel{ 0 }, pass{ 0 } {}
};

//注册回复
struct PROT_REGISTER_RS {
    packageType packtype;
    int result;
    PROT_REGISTER_RS() :packtype(DEF_REGISTER_RS), result(0) {}
};

//登录请求
struct PROT_LOGIN_RQ {
    packageType packtype;
    char tel[DEF_INFO_LEN];
    char pass[DEF_INFO_LEN];
    PROT_LOGIN_RQ() :packtype(DEF_LOGIN_RQ), tel{ 0 }, pass{ 0 } {}
};

//登录回复
struct PROT_LOGIN_RS {
    packageType packtype;
    int userId;
    int result;
    PROT_LOGIN_RS() :packtype(DEF_LOGIN_RS), userId(0), result(DEF_LOGIN_PASSERR) {}
};

//添加好友请求
struct PROT_ADD_FRIEND_RQ {
    packageType packtype;
    int userId;
    char usename[DEF_INFO_LEN];
    char friendName[DEF_INFO_LEN];

    PROT_ADD_FRIEND_RQ() :packtype(DEF_ADD_FRIEND_RQ), userId(0) {
        memset(usename, 0, DEF_INFO_LEN);
        memset(friendName, 0, DEF_INFO_LEN);
    }
};

//添加好友回复
struct PROT_ADD_FRIEND_RS {
    packageType packtype;
    int result;
    int userId;
    int friendId;
    char userName[DEF_INFO_LEN];
    char friendName[DEF_INFO_LEN];

    PROT_ADD_FRIEND_RS() :
        packtype(DEF_ADD_FRIEND_RS),
        result(DEF_ADD_FRIEND_NOTEXIST),
        userId(0),
        friendId(0) {
        memset(userName, 0, DEF_INFO_LEN);
        memset(friendName, 0, DEF_INFO_LEN);
    }
};

//聊天请求
struct PROT_CHAT_RQ {
    packageType packtype;
    int userId;
    int friendId;
    char content[DEF_CHATMSG_LEN];
    PROT_CHAT_RQ() : packtype(DEF_CHATMSG_RQ), userId(0), friendId(0) {
        memset(content, 0, DEF_CHATMSG_LEN);
    }
};

//聊天回复
struct PROT_CHAT_INFO_RS {
    packageType packtype;
    int friendId;
    int result;
    PROT_CHAT_INFO_RS() :packtype(DEF_CHATMSG_RS), friendId(0), result(DEF_SEND_CHAT_FA) {}
};

//好友下线
struct PROT_FRIEND_OFFLINE {
    packageType packtype;
    int userId;
    PROT_FRIEND_OFFLINE() :packtype(DEF_FRIEND_OFFLINE), userId(0) {}
};

//获取好友信息
struct PROT_FRIEND_INFO {
    packageType packtype;
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


// Create room request
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

// Create room reply
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

// Join room request
struct PROT_ROOM_JOIN_RQ {
    packageType packtype;
    int userId;
    int roomId;
    char roomPass[DEF_INFO_LEN];
    PROT_ROOM_JOIN_RQ() :packtype(DEF_ROOM_JOIN_RQ), userId(0), roomId(0) {
        memset(roomPass, 0, DEF_INFO_LEN);
    }
};

// Join room reply
struct PROT_ROOM_JOIN_RS {
    packageType packtype;
    int result;
    int roomId;
    int roomNumber;  // 8-digit room number
    int isCreator;   // 1=this user is the room creator
    char roomName[DEF_INFO_LEN];
    PROT_ROOM_JOIN_RS() :packtype(DEF_ROOM_JOIN_RS), result(DEF_ROOM_JOIN_FA), roomId(0), roomNumber(0), isCreator(0) {
        memset(roomName, 0, DEF_INFO_LEN);
    }
};

// Leave room request
struct PROT_ROOM_LEAVE_RQ {
    packageType packtype;
    int userId;
    int roomId;
    PROT_ROOM_LEAVE_RQ() :packtype(DEF_ROOM_LEAVE_RQ), userId(0), roomId(0) {}
};

// Leave room reply
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

// Room chat reply (broadcast to all members)
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

// Room list reply (server sends one per room the user is in)
struct PROT_ROOM_LIST_RS {
    packageType packtype;
    int roomId;
    int memberCount;
    int joined;
    int isCreator;  // 1=this user is the room creator
    int roomNumber;  // 8-digit room number
    char roomName[DEF_INFO_LEN];
    PROT_ROOM_LIST_RS() :packtype(DEF_ROOM_LIST_RS), roomId(0), memberCount(0), joined(0), isCreator(0), roomNumber(0) {
        memset(roomName, 0, DEF_INFO_LEN);
    }
};

// Room end meeting request (creator only)
struct PROT_ROOM_END_RQ {
    packageType packtype;
    int userId;
    int roomId;
    PROT_ROOM_END_RQ() :packtype(DEF_ROOM_END_RQ), userId(0), roomId(0) {}
};

// Room end meeting response (broadcast to all members)
struct PROT_ROOM_END_RS {
    packageType packtype;
    int result;
    int roomId;
    PROT_ROOM_END_RS() :packtype(DEF_ROOM_END_RS), result(0), roomId(0) {}
};

// Room member info (broadcast on join/leave)
struct PROT_ROOM_INFO {
    packageType packtype;
    int roomId;
    char roomName[DEF_INFO_LEN];
    int memberCount;
    int eventType;        // 0=JOIN, 1=LEAVE
    int userId;           // who joined/left
    char userName[DEF_INFO_LEN];  // who joined/left
    PROT_ROOM_INFO() :packtype(DEF_ROOM_INFO), roomId(0), memberCount(0),
                      eventType(0), userId(0) {
        memset(roomName, 0, DEF_INFO_LEN);
        memset(userName, 0, DEF_INFO_LEN);
    }
};


#define DEF_ROOM_MEMBER_MAX  (50)

struct PROT_ROOM_MEMBER_ITEM {
    int userId;
    char userName[DEF_INFO_LEN];
    int status;  // 0=in-room, 1=left
};

struct PROT_ROOM_MEMBER_LIST {
    packageType packtype;           // 1024
    int roomId;
    int memberCount;                // count in this packet
    int totalCount;                 // total members (for multi-packet)
    PROT_ROOM_MEMBER_ITEM members[DEF_ROOM_MEMBER_MAX];
    PROT_ROOM_MEMBER_LIST() :packtype(DEF_ROOM_MEMBER_LIST), roomId(0),
        memberCount(0), totalCount(0) {
        memset(members, 0, sizeof(members));
    }
};


// Audio frame (forwarded to all room members except sender)
#define DEF_AUDIO_FRAME_SIZE 800  // ~76 bytes actual, 200 for safety

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

// Video frame (forwarded to all room members except sender)
#define DEF_VIDEO_FRAME_SIZE 65536  // ~10-20KB JPEG at 320x240

struct PROT_VIDEO_FRAME {
    packageType packtype;
    int userId;
    int roomId;
    int frameSize;
    char data[DEF_VIDEO_FRAME_SIZE];
    PROT_VIDEO_FRAME() :packtype(DEF_VIDEO_FRAME), userId(0), roomId(0), frameSize(0) {
        memset(data, 0, DEF_VIDEO_FRAME_SIZE);
    }
};
