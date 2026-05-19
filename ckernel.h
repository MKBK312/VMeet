#ifndef CKERNEL_H
#define CKERNEL_H

#include <QObject>
#include <QSet>
#include<QTextCodec>
#include"chatdialog.h"
#include"./net/def.h"
#include"./Mediator/INetmediator.h"
#include"frienditem.h"
#include"friendlist.h"
#include"logindialog.h"
#include"./room/roomcreate.h"
#include<QMap>
#include<QTimer>
#include<QStringList>

class Audio_Read;
class Audio_Write;
class VideoCapture;
class ScreenCapture;

class CKernel;
//声明指向处理函数的指针
typedef void(CKernel::* PFUN)(char* data, int len,unsigned long from);
class CKernel : public QObject
{
    Q_OBJECT
public:
    explicit CKernel(QObject *parent = nullptr);
    ~CKernel();
    //给函数指针数组初始化并存数据
    void setProtocol();

    //gb2312转utf-8
    QString gb2312ToUtf8(char* src);
    //utf-8转gb2312
    void Utf8Togb2312(QString src,char* dst,int len);


    //处理注册回复
    void dealRegisterRs(char* data, int len, unsigned long from);

    //处理登录回复
    void dealLoginRs(char* data, int len, unsigned long from);

    //处理好友信息
    void dealFriendInfo(char* data, int len, unsigned long from);

    //处理聊天请求
    void dealChatRq(char* data, int len, unsigned long from);
    //处理聊天回复
    void dealChatRs(char* data, int len, unsigned long from);
    //处理下线请求
    void dealOfflineRq(char* data, int len, unsigned long from);
    //添加好友请求
    void dealAddFriRq(char* data, int len, unsigned long from);
    //添加好友回复
    void dealAddFriRs(char* data, int len, unsigned long from);


    // Room handlers
    void dealRoomCreateRs(char* data, int len, unsigned long from);
    void dealRoomJoinRs(char* data, int len, unsigned long from);
    void dealRoomLeaveRs(char* data, int len, unsigned long from);
    void dealRoomChatRs(char* data, int len, unsigned long from);
    void dealRoomChatRq(char* data, int len, unsigned long from);
    void dealRoomListRs(char* data, int len, unsigned long from);
    void dealRoomInfo(char* data, int len, unsigned long from);
    void dealRoomEndRs(char* data, int len, unsigned long from);
    void dealRoomMemberList(char* data, int len, unsigned long from);
    void dealAudioFrame(char* data, int len, unsigned long from);
    void dealVideoFrame(char* data, int len, unsigned long from);

    const QString &name() const;

signals:

private slots://处理所有接收到的数据
    void slot_delData(char* data, int len, unsigned long from);
    //让Kernel发给服务端
    //注册数据
    void slots_registerData(QString name,QString tel,QString pass);
    //登录数据
    void slots_loginData(QString tel,QString pass);
    //显示当前好友聊天窗口
    void slots_showChatDialog(int friendId);
    //处理聊天内容，发给服务端
    void slots_chatMessage(int friendId,QString content);
    //负责关闭程序的信号
    void slots_closeProcess();
    //处理下线信号
    void slots_offline();
    //处理添加好友信号
    void slots_addFriend();

    // Room UI slots
    void slots_createRoom(QString roomName, QString roomPass);
    void slots_joinRoom(int roomId, QString roomPass);
    void slots_leaveRoom(int roomId);
    void slots_roomChatMessage(int roomId, QString content);
    void slots_roomListRequest();
    void slots_showRoomCreate();
    void slots_showRoomJoin();
    void slots_showRoomJoinById(int roomId);
    void slots_removeRoomFromList(int roomId);
    void slots_enterRoom(int roomId);
    void slots_endMeeting(int roomId);
    void slots_copyRoomNumber();
    void slots_startRoomMedia(int roomId);
    void slots_stopRoomMedia();
    void slots_sendAudioFrame(QByteArray ba);
    void slots_sendVideoFrame(QByteArray ba);
    void slots_toggleMute(bool muted);
    void slots_toggleCamera(bool off);
    void slots_toggleScreenShare();
private:
    int m_id;
    QString m_name;
    LoginDialog* m_pLoginDlg;
    INetMediator* m_pMediator;
    friendList* m_pFriendList;
    //声明函数指针数组
    PFUN m_protocol[DEF_PROTOCOL_COUNT];
    //保存好友的对象frienditem
    QMap<int,frienditem*>m_mapIdToFriendItem;
    //保存跟好友的聊天窗口的对象
    QMap<int,ChatDialog*>m_mapIdToChatDlg;

    // Room metadata (for room name lookup)
    struct RoomData {
        int roomId;
        int roomNumber;
        QString roomName;
        int memberCount;
        int joined;
        RoomData() : roomId(0), roomNumber(0), memberCount(0), joined(0) {}
    };
    QMap<int, RoomData*> m_mapRoomData;

    QSet<int> m_setCreatedRooms;  // roomIds created by this user
    QSet<int> m_setEverJoinedRooms;  // roomIds user has ever joined
    QSet<int> m_setRemovedRooms;  // rooms explicitly removed from lobby by user

    // Cached room chat messages for non-viewed rooms
    // Key: roomId, Value: list of "senderName\tcontent" strings
    QMap<int, QStringList> m_mapPendingRoomMessages;

    QTimer* m_pRoomPollTimer;  // polls room list while in a room

    Audio_Read*  m_pAudioRead;
    Audio_Write* m_pAudioWrite;
    VideoCapture* m_pVideoCapture;
    ScreenCapture* m_pScreenCapture;
    bool m_screenSharing;
    int m_mediaRoomId;
};

#endif // CKERNEL_H
