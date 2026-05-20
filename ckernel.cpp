#include "ckernel.h"
#include"Mediator/TcpClientMediator.h"
#include<QMessageBox>
#include<QInputDialog>
#include<QDialog>
#include<QVBoxLayout>
#include<QHBoxLayout>
#include<QLabel>
#include<QLineEdit>
#include<QPushButton>
#include<QSpacerItem>
#include<QDebug>
#include<QIntValidator>
#include<iostream>
#include"audio_read.h"
#include"audio_write.h"
#include"video/video_capture.h"
#include"video/screen_capture.h"
#include"video/video_encoder.h"
#include"video/video_decoder.h"
#include"audio/audio_encoder.h"
#include"audio/audio_decoder.h"
#include<QBuffer>
using namespace std;
CKernel::CKernel(QObject *parent) : QObject(parent)
{
    cout << __func__ << " start" << endl;
    setProtocol();
    cout << __func__ << " after setProtocol" << endl;
    // Room polling timer — keeps lobby member count updated in real-time
    m_pRoomPollTimer = new QTimer(this);
    m_pRoomPollTimer->setInterval(5000);
    connect(m_pRoomPollTimer, &QTimer::timeout, this, &CKernel::slots_roomListRequest);

    m_pAudioRead = nullptr;
    m_pAudioWrite = nullptr;
    m_pVideoCapture = nullptr;
    m_pScreenCapture = nullptr;
    m_pVideoEncoder = nullptr;
    m_pVideoDecoder = nullptr;
    m_pAudioEncoder = nullptr;
    m_pAudioDecoder = nullptr;
    m_screenSharing = false;
    m_mediaRoomId = 0;

    //new一个好友列表见面的对象
    m_pFriendList=new friendList;
    cout << __func__ << " after new friendList" << endl;

    //new一个登录窗口类的对象
    m_pLoginDlg=new LoginDialog;
    cout << __func__ << " after new LoginDialog" << endl;
    //显示窗口
    m_pLoginDlg->show();
    cout << __func__ << " after show LoginDialog" << endl;

    //绑定处理注册信息的的信号和槽函数
    connect(m_pLoginDlg,&LoginDialog::sig_registerData,
            this,&CKernel::slots_registerData);
    connect(m_pLoginDlg,&LoginDialog::sig_loginData,
            this,&CKernel::slots_loginData);
    connect(m_pLoginDlg,&LoginDialog::sig_closeProcess,
            this,&CKernel::slots_closeProcess);
    connect(m_pFriendList,&friendList::sig_offline,
            this,&CKernel::slots_offline);
    connect(m_pFriendList,&friendList::sig_addFriend,
            this,&CKernel::slots_addFriend);

    // Meeting signals — all routed through friendList embedded UI
    connect(m_pFriendList,&friendList::sig_createRoom,
            this,&CKernel::slots_showRoomCreate);
    connect(m_pFriendList,&friendList::sig_joinRoom,
            this,&CKernel::slots_showRoomJoin);
    connect(m_pFriendList,&friendList::sig_enterRoom,
            this,&CKernel::slots_enterRoom);
    connect(m_pFriendList,&friendList::sig_leaveRoom,
            this,&CKernel::slots_leaveRoom);
    connect(m_pFriendList,&friendList::sig_roomChatMessage,
            this,&CKernel::slots_roomChatMessage);
    connect(m_pFriendList,&friendList::sig_joinRoomById,
            this,&CKernel::slots_showRoomJoinById);
    connect(m_pFriendList,&friendList::sig_removeRoomFromList,
            this,&CKernel::slots_removeRoomFromList);
    connect(m_pFriendList,&friendList::sig_endMeeting,
            this,&CKernel::slots_endMeeting);
    connect(m_pFriendList,&friendList::sig_toggleMute,
            this,&CKernel::slots_toggleMute);
    connect(m_pFriendList,&friendList::sig_toggleCamera,
            this,&CKernel::slots_toggleCamera);
    connect(m_pFriendList,&friendList::sig_toggleScreenShare,
            this,&CKernel::slots_toggleScreenShare);

    cout << __func__ << " after connects" << endl;
    //new一个中介者类的对象
    m_pMediator=new TcpClientMediator;
    cout << __func__ << " after new TcpClientMediator" << endl;
    //连接处理所有数据的信号和槽（最好new完就链接）
    connect(m_pMediator,&TcpClientMediator::sig_dealData,this,&CKernel::slot_delData);
    cout << __func__ << " after connect mediator" << endl;
    //打开网络
    if(!m_pMediator->openNet())
    {
    //弹窗提示打开网络失败
    //提示打开网络失败
       QMessageBox::about(m_pLoginDlg/*弹出窗口的父窗口，觉得窗口的位置*/,
                          "its a test"/*弹出窗口的标题*/,
                          "打开网络失败"//提示信息
                          );
        //退出程序
       exit(1);
    }
    cout << __func__ << " after openNet - constructor done" << endl;

}

CKernel::~CKernel()
{
    //回收窗口
    if(m_pLoginDlg)
    {
        m_pLoginDlg->hide();
        delete m_pLoginDlg;
        m_pLoginDlg=nullptr;
    }

    if(m_pMediator)
    {
        m_pMediator->closeNet();
        delete m_pMediator;
        m_pMediator=nullptr;
    }
    if(m_pFriendList)
    {
        m_pFriendList->hide();
        delete  m_pFriendList;
        m_pFriendList=nullptr;
    }
    // Clean up room metadata
    for(QMap<int, RoomData*>::iterator it = m_mapRoomData.begin(); it != m_mapRoomData.end();)
    {
        delete it.value();
        it = m_mapRoomData.erase(it);
    }
    // Clean up chat dialogs
    for(QMap<int,ChatDialog*>::iterator it = m_mapIdToChatDlg.begin(); it != m_mapIdToChatDlg.end();)
    {
        ChatDialog* chat = it.value();
        if(chat) { chat->hide(); delete chat; }
        it = m_mapIdToChatDlg.erase(it);
    }
    delete m_pVideoEncoder; m_pVideoEncoder = nullptr;
    delete m_pVideoDecoder; m_pVideoDecoder = nullptr;
    delete m_pAudioEncoder; m_pAudioEncoder = nullptr;
    delete m_pAudioDecoder; m_pAudioDecoder = nullptr;
}

void CKernel::setProtocol()
{
    qDebug()<< __func__;
    //初始化成0
    memset(m_protocol, 0, sizeof(m_protocol));
    //存入数据
    m_protocol[DEF_REGISTER_RS -      DEF_PROTOCOL_BASE ] = &CKernel::dealRegisterRs;
    m_protocol[DEF_LOGIN_RS -         DEF_PROTOCOL_BASE ] = &CKernel::dealLoginRs;
    m_protocol[DEF_FRIEND_INFO -      DEF_PROTOCOL_BASE ] = &CKernel::dealFriendInfo;
    m_protocol[DEF_CHATMSG_RQ -       DEF_PROTOCOL_BASE ] = &CKernel::dealChatRq;
    m_protocol[DEF_CHATMSG_RS -       DEF_PROTOCOL_BASE ] = &CKernel::dealChatRs;
    m_protocol[DEF_FRIEND_OFFLINE -   DEF_PROTOCOL_BASE ] = &CKernel::dealOfflineRq;
    m_protocol[DEF_ADD_FRIEND_RQ -   DEF_PROTOCOL_BASE ] = &CKernel::dealAddFriRq;
    m_protocol[DEF_ADD_FRIEND_RS -   DEF_PROTOCOL_BASE ] = &CKernel::dealAddFriRs;

    // Room protocols
    m_protocol[DEF_ROOM_CREATE_RS -  DEF_PROTOCOL_BASE ] = &CKernel::dealRoomCreateRs;
    m_protocol[DEF_ROOM_JOIN_RS -    DEF_PROTOCOL_BASE ] = &CKernel::dealRoomJoinRs;
    m_protocol[DEF_ROOM_LEAVE_RS -   DEF_PROTOCOL_BASE ] = &CKernel::dealRoomLeaveRs;
    m_protocol[DEF_ROOM_CHAT_RS -    DEF_PROTOCOL_BASE ] = &CKernel::dealRoomChatRs;
    m_protocol[DEF_ROOM_CHAT_RQ -    DEF_PROTOCOL_BASE ] = &CKernel::dealRoomChatRq;
    m_protocol[DEF_ROOM_LIST_RS -    DEF_PROTOCOL_BASE ] = &CKernel::dealRoomListRs;
    m_protocol[DEF_ROOM_INFO -       DEF_PROTOCOL_BASE ] = &CKernel::dealRoomInfo;
    m_protocol[DEF_ROOM_END_RS -     DEF_PROTOCOL_BASE ] = &CKernel::dealRoomEndRs;
    m_protocol[DEF_ROOM_MEMBER_LIST - DEF_PROTOCOL_BASE ] = &CKernel::dealRoomMemberList;
    m_protocol[DEF_AUDIO_FRAME - DEF_PROTOCOL_BASE ] = &CKernel::dealAudioFrame;
    m_protocol[DEF_VIDEO_FRAME - DEF_PROTOCOL_BASE ] = &CKernel::dealVideoFrame;
}

QString CKernel::gb2312ToUtf8(char *src)
{
    QTextCodec* dc=QTextCodec::codecForName("gb2312");
    return dc->toUnicode(src);
}

void CKernel::Utf8Togb2312(QString src, char *dst, int len)
{
    // P2-1 fix: replace strcpy_s with safe memcpy + truncation
    QTextCodec* dc=QTextCodec::codecForName("gb2312");
    QByteArray ba=dc->fromUnicode(src);
    int copyLen = qMin(ba.size(), len - 1);
    memcpy(dst, ba.constData(), copyLen);
    dst[copyLen] = '\0';
}

void CKernel::dealRegisterRs(char *data, int len, unsigned long from)
{

    qDebug()<<__func__;
    PROT_REGISTER_RS* rs=(PROT_REGISTER_RS*)data;
    switch (rs->result) {
    case DEF_REGISTER_SUC:
        QMessageBox::about(m_pLoginDlg,"提示","注册成功");
        break;
    case DEF_REGISTER_TEL_EXIST:
        QMessageBox::about(m_pLoginDlg,"提示","电话号码已被注册");
        break;
    case DEF_REGISTER_NAME_EXIST:
        QMessageBox::about(m_pLoginDlg,"提示","名字已被注册");
        break;
    }
}

void CKernel::dealLoginRs(char *data, int len, unsigned long from)
{

    qDebug()<<__func__;
    PROT_LOGIN_RS* rs=(PROT_LOGIN_RS*)data;
    switch (rs->result) {
    case DEF_LOGIN_SUC:
    {
        //保存当前用户的id
        m_id=rs->userId;
        //隐藏登录界面，显示好友界面
        m_pLoginDlg->hide();
        m_pFriendList->show();
        // 登录成功后自动加载会议列表
        slots_roomListRequest();
    }
        break;
    case DEF_LOGIN_USERNOTEXIST:
        QMessageBox::about(m_pLoginDlg,"提示","用户不存在");
        break;
    case DEF_LOGIN_PASSERR:
        QMessageBox::about(m_pLoginDlg,"提示","密码错误");
        break;
    case 3:  // Account already logged in on another client
        QMessageBox::about(m_pLoginDlg, "提示", "该账户已在其他设备登录");
        m_pMediator->closeNet();
        if (!m_pMediator->openNet()) {
            qDebug() << "dealLoginRs: re-openNet failed after close";
            QMessageBox::about(m_pLoginDlg, "提示", "网络重连失败，请重启程序");
            exit(1);
        }
        qDebug() << "dealLoginRs: re-login ready, showing login dialog";
        m_pFriendList->hide();
        m_pLoginDlg->show();
        break;
    }

}

void CKernel::dealFriendInfo(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_FRIEND_INFO* info=(PROT_FRIEND_INFO*)data;
    QString name=gb2312ToUtf8(info->name);
    QString feeling=gb2312ToUtf8(info->feeling);


    //2、判断是不是自己信息
    if(m_id==info->id)
    {
        //把自己的名字取出来
        m_name=name;
        //把自己的信息设置到用户界面上
        m_pFriendList->setUserInfo(name,feeling,info->iconId);
        return;
    }

    //是好友的信息，判断是非已经添加到列表上了
    if(m_mapIdToFriendItem.count(info->id)>0)
    {
        //如果已经在列表上
        //取出好友的对象
        frienditem* item=m_mapIdToFriendItem[info->id];
        //更新好友的信息
        item->setFriendInfo(info->id,name,feeling,info->iconId,info->status);
    }else{
        //没有在列表上
        //new一个好友的对象
        frienditem* item=new frienditem;
        //绑定显示聊天窗口的信号和槽函数
        connect(item,&frienditem::sig_showChatDialog,
                this,&CKernel::slots_showChatDialog);
        //把好友添加到到列表上
        item->setFriendInfo(info->id,name,feeling,info->iconId,info->status);
        m_pFriendList->addFriend(item);
        //保存好友对象friendItm
        m_mapIdToFriendItem[info->id]=item;

        //new一个跟这个好友的聊天窗口
        ChatDialog* chat=new ChatDialog;
        //设置聊天窗口属性
        chat->setFriendinfo(info->id,name);
        //把聊天窗口保存到map中
        m_mapIdToChatDlg[info->id]=chat;

        //绑定发送聊天的信号和槽函数
        connect(chat,&ChatDialog::sig_chatMessage,
                this,&CKernel::slots_chatMessage);
    }




}

void CKernel::dealChatRq(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_CHAT_RQ* rq=(PROT_CHAT_RQ*)data;
    qDebug()<<"2:"<<rq->content;
    //判断与A窗口是否存在
    if(m_mapIdToChatDlg.count(rq->userId)>0)
    {
        //取出A窗口
        ChatDialog* chat=m_mapIdToChatDlg[rq->userId];
        //把聊天内容设置到窗口上
        chat->setChatMessage(rq->content);
        //显示窗口
        chat->show();

    }
}

void CKernel::dealChatRs(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_CHAT_INFO_RS* rs=(PROT_CHAT_INFO_RS*)data;
    if(m_mapIdToChatDlg.count(rs->friendId)>0)
    {
        //取出B窗口
        ChatDialog* chat=m_mapIdToChatDlg[rs->friendId];
        //把聊天内容设置到窗口上
        chat->setFriOffline();
        //显示窗口
        chat->show();

    }
}
//处理下线请求（是客户端B，好友A下线了）
void CKernel::dealOfflineRq(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_FRIEND_OFFLINE* rq=(PROT_FRIEND_OFFLINE*)data;

    //找到好友A的头像item
    if(m_mapIdToFriendItem.count(rq->userId)>0)
    {
        frienditem* item=m_mapIdToFriendItem[rq->userId];
        item->setFriendOffline();
    }
}

void CKernel::dealAddFriRq(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_ADD_FRIEND_RQ* rq=(PROT_ADD_FRIEND_RQ*)data;
    //弹出询问窗口，是否同意添加好友
    PROT_ADD_FRIEND_RS rs;
    rs.userId=rq->userId;
    rs.friendId=m_id;
    strcpy_s(rs.userName,sizeof (rs.userName),rq->username);
    strcpy_s(rs.friendName,sizeof (rs.friendName),rq->friendName);
    if(QMessageBox::Yes==QMessageBox::question(m_pFriendList,"添加好友",QString("【%1】想添加你为好友，是否同意").arg(rq->username))){
        rs.result=DEF_ADD_FRIEND_ACCEPT;
    }else{
        rs.result=DEF_ADD_FRIEND_REJECT;
    }
    m_pMediator->sendData((char*)&rs,sizeof(rs),123);

}

void CKernel::dealAddFriRs(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //1拆包
    PROT_ADD_FRIEND_RS* rs=(PROT_ADD_FRIEND_RS*)data;
    QString friendName=gb2312ToUtf8(rs->friendName);

    //根据添加结果提示用户
    switch (rs->result)
    {
    case DEF_ADD_FRIEND_ACCEPT:
        QMessageBox::about(m_pFriendList,"提示",QString("添加【%1】为好友成功").arg(rs->friendName));
        break;
    case DEF_ADD_FRIEND_OFFLINE:
        QMessageBox::about(m_pFriendList,"提示",QString("添加【%1】为好友失败，好友不在线").arg(friendName));
        break;
    case DEF_ADD_FRIEND_REJECT:
        QMessageBox::about(m_pFriendList,"提示",QString("添加【%1】为好友失败，好友拒绝").arg(rs->friendName));
        break;
    case DEF_ADD_FRIEND_NOTEXIST:
        QMessageBox::about(m_pFriendList,"提示",QString("添加【%1】为好友失败，好友不存在").arg(friendName));
        break;
    default:
        break;
    }


}


//处理所有接受的数据
void CKernel::slot_delData(char *data, int len, unsigned long from)
{
    qDebug()<< __func__;
    //取出协议头
    packageType type = *(packageType*)data;

    int index = type - DEF_PROTOCOL_BASE;
    if (index >= 0 && index < DEF_PROTOCOL_COUNT)
    {
        //根据数组下标，取出函数地址
        PFUN pf = m_protocol[index];
        if (pf)
        {
            //调用具体的处理函数
            (this->*pf)(data, len, from);
        }
        else {
            //2：1、定义结构体时候，type值错了；2、对端发送的结构体不对
            qDebug() << "type2:" << type ;
        }

    }
    else {
        //打印type1：1、声明结构体的时候packtype没有第一个声明；2、noffset没有清零
        qDebug() << "type1:" << type;
    }

    //回收空间
    delete[] data;

}
//处理注册数据，发给服务端
void CKernel::slots_registerData(QString name, QString tel, QString pass)
{
    qDebug()<<__func__;
    //打包
    PROT_REGISTER_RQ rq;
    strcpy_s(rq.name,   sizeof (rq.name),  name.toStdString().c_str());
    strcpy_s(rq.pass,   sizeof (rq.pass),  pass.toStdString().c_str());
    strcpy_s(rq.tel,    sizeof (rq.tel),   tel.toStdString().c_str());
    //发给服务端
    m_pMediator->sendData((char*)&rq,sizeof(rq),89);
}
//处理登录数据，发给服务端
void CKernel::slots_loginData( QString tel, QString pass)
{
    qDebug()<<__func__;
    //打包
    PROT_LOGIN_RQ rq;
    strcpy_s(rq.pass,   sizeof (rq.pass),  pass.toStdString().c_str());
    strcpy_s(rq.tel,    sizeof (rq.tel),   tel.toStdString().c_str());
    qDebug()<<rq.tel<<" "<<rq.pass;
    //发给服务端
    if (!m_pMediator->sendData((char*)&rq, sizeof(rq), 89)) {
        qDebug() << "slots_loginData: sendData failed, trying reconnect";
        m_pMediator->closeNet();
        if (!m_pMediator->openNet()) {
            QMessageBox::about(m_pLoginDlg, "提示", "网络连接失败，请检查网络后重试");
            return;
        }
        // Retry send
        if (!m_pMediator->sendData((char*)&rq, sizeof(rq), 89)) {
            QMessageBox::about(m_pLoginDlg, "提示", "登录失败，请检查网络后重试");
            return;
        }
    }
}

void CKernel::slots_showChatDialog(int friendId)
{
    qDebug()<<__func__;
    //判断有没有聊天窗口，有就显示
    if(m_mapIdToChatDlg.contains(friendId))
    {
        ChatDialog* chat=m_mapIdToChatDlg[friendId];
        chat->show();
    }

}

void CKernel::slots_chatMessage(int friendId, QString content)
{

    qDebug()<<__func__;
    //打包
    PROT_CHAT_RQ rq;
    rq.userId=m_id;
    rq.friendId=friendId;
    // Safe copy with truncation (P2-1 fix)
    QByteArray bytes = content.toUtf8();
    size_t copyLen = qMin((size_t)bytes.size(), sizeof(rq.content) - 1);
    memcpy(rq.content, bytes.constData(), copyLen);
    rq.content[copyLen] = '\0';
    qDebug()<<"1:"<<rq.content;

    //发给服务端
    m_pMediator->sendData((char*)&rq,sizeof(rq),89);
}

void CKernel::slots_closeProcess()
{
    qDebug()<<__func__;
    qDebug() << "[DEBUG slots_closeProcess] m_setCreatedRooms (about to be lost):" << m_setCreatedRooms;
    //回收资源
    if(m_pLoginDlg)
    {
        m_pLoginDlg->hide();
        delete m_pLoginDlg;
        m_pLoginDlg=nullptr;
    }

    if(m_pMediator)
    {
        m_pMediator->closeNet();
        delete m_pMediator;
        m_pMediator=nullptr;
    }
    if(m_pFriendList)
    {
        m_pFriendList->hide();
        delete  m_pFriendList;
        m_pFriendList=nullptr;
    }
    // Clean up room metadata
    for(QMap<int, RoomData*>::iterator it = m_mapRoomData.begin(); it != m_mapRoomData.end();)
    {
        delete it.value();
        it = m_mapRoomData.erase(it);
    }
    for(QMap<int,ChatDialog*>::iterator ite=m_mapIdToChatDlg.begin();ite!=m_mapIdToChatDlg.end();)
    {
        //取出节点中的窗口对象
        ChatDialog* chat=ite.value();
        //回收窗口对象
        if(chat)
        {
            chat->hide();
            delete chat;
            chat=nullptr;
        }
        //把无效节点从map中移除
        ite=m_mapIdToChatDlg.erase(ite);
    }
    //退出程序
    exit(0);

}

void CKernel::slots_offline()
{
    qDebug()<<__func__;
    qDebug() << "[DEBUG slots_offline] m_setCreatedRooms:" << m_setCreatedRooms;

    // End all created rooms first (notifies members "会议已结束")
    for (int roomId : m_setCreatedRooms) {
        qDebug() << "[DEBUG slots_offline] sending END_RQ for room:" << roomId;
        PROT_ROOM_END_RQ endRq;
        endRq.userId = m_id;
        endRq.roomId = roomId;
        m_pMediator->sendData((char*)&endRq, sizeof(endRq), 89);
    }

    //给服务器发送下线请求
    PROT_FRIEND_OFFLINE rq;
    rq.userId=m_id;
    m_pMediator->sendData((char*)&rq,sizeof(rq),67);

    //回收资源退出进程
    slots_closeProcess();
}

void CKernel::  slots_addFriend()
{
    qDebug()<<__func__;
    //弹出一个输入窗口，让用户输入好友昵称
    QString name=QInputDialog::getText(m_pFriendList,"添加好友","请输入好友的昵称：");
    QString nameTmp=name;
    //判断字符串是否为全空或是全空格
    if(name.isEmpty()||nameTmp.remove(" ").isEmpty())
    {
        QMessageBox::about(m_pFriendList,"提示","请输入正确好友昵称");
        return;
    }
    //判断是否是自己的昵称
    if(m_name==name)
    {
        QMessageBox::about(m_pFriendList,"提示","不能添加自己为好友");
        return;
    }
    //判断是否已经是好友了
    for(QMap<int,frienditem*>::iterator ite=m_mapIdToFriendItem.begin();ite!=m_mapIdToFriendItem.end();ite++)
    {
        //取出好友的friendItem
        frienditem* item=ite.value();
        //判断好友的昵称和输入的昵称是否相同
        if(name==item->name()){
            QMessageBox::about(m_pFriendList,"提示","已经是好友，不需要重复添加");
            return;
        }
    }
    //给服务端发送添加好友请求
    PROT_ADD_FRIEND_RQ rq;
    rq.userId=m_id;
    strcpy_s(rq.friendName, sizeof(rq.friendName), name.toStdString().c_str());
    strcpy_s(rq.username,sizeof(rq.username),m_name.toStdString().c_str());
    m_pMediator->sendData((char*)&rq,sizeof(rq),2);




}

// ========== Room Handlers ==========

void CKernel::dealRoomCreateRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__;
    PROT_ROOM_CREATE_RS* rs = (PROT_ROOM_CREATE_RS*)data;
    if (rs->result == DEF_ROOM_CREATE_SUC) {
        m_setCreatedRooms.insert(rs->roomId);
        // Store room data
        QString roomName = gb2312ToUtf8(rs->roomName);
        RoomData* roomData = new RoomData;
        roomData->roomId = rs->roomId;
        roomData->roomNumber = rs->roomNumber;
        roomData->roomName = roomName;
        roomData->memberCount = 1;
        roomData->joined = 1;  // creator is always a member
        m_mapRoomData[rs->roomId] = roomData;

        // Switch to embedded room view FIRST (before modal dialog)
        m_pFriendList->showRoomView(rs->roomId, rs->roomNumber, roomName, true);

        // Refresh room list in lobby (creator is already a member)
        m_pFriendList->addRoomToLobby(rs->roomId, rs->roomNumber, roomName, 1, 1);
        slots_roomListRequest();
        m_pRoomPollTimer->start();  // start polling for real-time member count
        slots_startRoomMedia(rs->roomId);

        // Show success message LAST
        QMessageBox::about(m_pFriendList, "提示", QString("房间创建成功！房间号为：%1").arg(rs->roomNumber));
    } else {
        QMessageBox::about(m_pFriendList, "提示", "房间创建失败");
    }
}

void CKernel::dealRoomJoinRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__ << "len:" << len << "sizeof(PROT_ROOM_JOIN_RS):" << sizeof(PROT_ROOM_JOIN_RS);
    PROT_ROOM_JOIN_RS* rs = (PROT_ROOM_JOIN_RS*)data;
    QString roomName = gb2312ToUtf8(rs->roomName);
    switch (rs->result) {
    case DEF_ROOM_JOIN_SUC:
    {
        // Store room data
        if (m_mapRoomData.contains(rs->roomId)) {
            delete m_mapRoomData[rs->roomId];
        }
        RoomData* roomData = new RoomData;
        roomData->roomId = rs->roomId;
        roomData->roomNumber = rs->roomNumber;
        roomData->roomName = roomName;
        roomData->memberCount = 1;  // initial; updated by roomListRequest
        roomData->joined = 1;  // just joined successfully
        m_mapRoomData[rs->roomId] = roomData;

        // Track that this user has joined this room
        m_setEverJoinedRooms.insert(rs->roomId);

        // Use server-provided isCreator to rebuild state (survives logout)
        if (rs->isCreator) {
            m_setCreatedRooms.insert(rs->roomId);
        }

        // Re-allow room in lobby if it was previously removed by user
        m_setRemovedRooms.remove(rs->roomId);

        // Always update room view with fresh data from server
        // (handles re-join case where slots_enterRoom showed stale defaults)
        bool isCreator = (rs->isCreator != 0);
        qDebug() << "[DEBUG dealRoomJoinRs] roomId:" << rs->roomId << "server_isCreator:" << rs->isCreator;
        bool alreadyViewing = (m_pFriendList->getCurrentRoomId() == rs->roomId);
        m_pFriendList->showRoomView(rs->roomId, rs->roomNumber, roomName, isCreator);

        // Refresh room list
        slots_roomListRequest();
        m_pRoomPollTimer->start();  // start polling for real-time member count
        slots_startRoomMedia(rs->roomId);

        // Show success message only for explicit joins (not re-enter)
        if (!alreadyViewing) {
            QMessageBox::about(m_pFriendList, "提示", QString("加入房间【%1】成功").arg(roomName));
        }
        break;
    }
    case DEF_ROOM_JOIN_PASSERR:
        QMessageBox::about(m_pFriendList, "提示", "密码错误");
        break;
    case DEF_ROOM_JOIN_FA:
        // Room no longer exists (e.g. ended by host) — clean up local state
        m_setEverJoinedRooms.remove(rs->roomId);
        m_setCreatedRooms.remove(rs->roomId);
        if (m_mapRoomData.contains(rs->roomId)) {
            delete m_mapRoomData[rs->roomId];
            m_mapRoomData.remove(rs->roomId);
        }
        slots_roomListRequest();
        QMessageBox::about(m_pFriendList, "提示", "房间不存在");
        break;
    }
}

void CKernel::dealRoomLeaveRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__;
    PROT_ROOM_LEAVE_RS* rs = (PROT_ROOM_LEAVE_RS*)data;
    if (rs->result == DEF_ROOM_LEAVE_SUC) {
        m_pRoomPollTimer->stop();  // stop polling since we left
        slots_stopRoomMedia();
        QMessageBox::about(m_pFriendList, "提示", "已退出房间");

        // Remove from local data
        if (m_mapRoomData.contains(rs->roomId)) {
            delete m_mapRoomData[rs->roomId];
            m_mapRoomData.remove(rs->roomId);
        }
        // Keep m_setEverJoinedRooms — room should still appear in lobby after leaving
        // Note: do NOT remove from m_setCreatedRooms here — creator needs End Meeting button on re-entry
        qDebug() << "[DEBUG dealRoomLeaveRs] roomId:" << rs->roomId << "m_setCreatedRooms:" << m_setCreatedRooms << "m_setEverJoinedRooms:" << m_setEverJoinedRooms;

        // Return to lobby
        m_pFriendList->showRoomLobby();

        // Refresh room list
        slots_roomListRequest();
    }
}

void CKernel::dealRoomEndRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__ << "roomId:" << ((PROT_ROOM_END_RS*)data)->roomId;
    m_pRoomPollTimer->stop();  // stop polling since meeting ended
    slots_stopRoomMedia();
    PROT_ROOM_END_RS* rs = (PROT_ROOM_END_RS*)data;

    // Clean up local data
    if (m_mapRoomData.contains(rs->roomId)) {
        delete m_mapRoomData[rs->roomId];
        m_mapRoomData.remove(rs->roomId);
    }
    m_setCreatedRooms.remove(rs->roomId);
    m_setEverJoinedRooms.remove(rs->roomId);

    // Show end notification and return to lobby
    m_pFriendList->showRoomLobby();
    QMessageBox::about(m_pFriendList, "提示", "会议已结束");

    // Refresh room list
    slots_roomListRequest();
}

void CKernel::dealRoomMemberList(char* data, int len, unsigned long from)
{
    qDebug() << __func__;
    PROT_ROOM_MEMBER_LIST* list = (PROT_ROOM_MEMBER_LIST*)data;

    if (m_mapRoomData.contains(list->roomId)) {
        m_mapRoomData[list->roomId]->memberCount = list->totalCount;
    }

    QStringList memberNames;
    QList<int> memberIds;
    for (int i = 0; i < list->memberCount && i < DEF_ROOM_MEMBER_MAX; ++i) {
        QString name = gb2312ToUtf8(list->members[i].userName);
        memberNames.append(name);
        memberIds.append(list->members[i].userId);
    }
    m_pFriendList->updateRoomMembers(list->roomId, memberNames, memberIds, list->totalCount);
}

void CKernel::dealRoomChatRq(char *data, int len, unsigned long from)
{
    // 客户端不接收此协议（服务端不发送1017），保留空实现防止协议表中出现空指针
    qDebug() << __func__ << "unused - server sends RS not RQ";
    (void)data; (void)len; (void)from;
}

void CKernel::dealRoomChatRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__;
    // 1、拆包
    PROT_ROOM_CHAT_RS* rs = (PROT_ROOM_CHAT_RS*)data;
    if (rs->result != DEF_ROOM_CHAT_SUC) {
        QMessageBox::about(m_pFriendList, "提示", "消息发送失败");
        return;
    }
    // 2、跳过自己发送的消息（已在本地显示）
    if (rs->userId == (unsigned int)m_id) {
        return;
    }
    // 3、过滤：只显示当前房间的消息，非当前房间的消息缓存起来
    int currentRoom = m_pFriendList->getCurrentRoomId();
    if (rs->roomId != currentRoom) {
        // Cache message for later display when user switches to this room
        QString senderName = gb2312ToUtf8(rs->senderName);
        QString content = QString::fromUtf8(rs->content);
        m_mapPendingRoomMessages[rs->roomId].append(senderName + "\t" + content);
        qDebug() << __func__ << "cached msg for room" << rs->roomId << "from" << senderName;
        return;
    }
    // 4、显示消息到嵌入式房间视图
    QString senderName = gb2312ToUtf8(rs->senderName);
    m_pFriendList->displayRoomMessage(senderName, rs->content);
}

void CKernel::dealRoomListRs(char *data, int len, unsigned long from)
{
    qDebug() << __func__;
    // 1、拆包
    PROT_ROOM_LIST_RS* rs = (PROT_ROOM_LIST_RS*)data;
    QString roomName = gb2312ToUtf8(rs->roomName);

    // Store room data
    if (m_mapRoomData.contains(rs->roomId)) {
        delete m_mapRoomData[rs->roomId];
    }
    RoomData* roomData = new RoomData;
    roomData->roomId = rs->roomId;
    roomData->roomNumber = rs->roomNumber;
    roomData->roomName = roomName;
    roomData->memberCount = rs->memberCount;
    roomData->joined = rs->joined;
    m_mapRoomData[rs->roomId] = roomData;

    // Rebuild creator set from server-provided isCreator (survives logout)
    if (rs->isCreator) {
        m_setCreatedRooms.insert(rs->roomId);
    }

    // Show rooms user is currently in, has created, or has left (can re-join)
    // everJoined is cleared only by dealRoomEndRs (meeting truly ended)
    bool isOwnRoom = (rs->isCreator != 0) || m_setCreatedRooms.contains(rs->roomId);
    bool everJoined = m_setEverJoinedRooms.contains(rs->roomId);
    bool removed = m_setRemovedRooms.contains(rs->roomId);
    if ((rs->joined || isOwnRoom || everJoined) && !removed) {
        m_pFriendList->addRoomToLobby(rs->roomId, rs->roomNumber, roomName, rs->memberCount,
                                       (rs->joined || isOwnRoom) ? 1 : 0);
    }
}

void CKernel::dealRoomInfo(char *data, int len, unsigned long from)
{
    qDebug() << __func__;
    // 1、拆包 — 服务端广播成员变更通知
    PROT_ROOM_INFO* info = (PROT_ROOM_INFO*)data;
    QString roomName = gb2312ToUtf8(info->roomName);

    // Store/update room data in-place (avoid delete+new)
    bool isNewRoom = !m_mapRoomData.contains(info->roomId);
    RoomData* roomData = NULL;
    if (isNewRoom) {
        roomData = new RoomData;
        roomData->roomId = info->roomId;
        m_mapRoomData[info->roomId] = roomData;
    } else {
        roomData = m_mapRoomData[info->roomId];
    }
    int oldCount = isNewRoom ? 0 : roomData->memberCount;
    roomData->roomName = roomName;
    roomData->memberCount = info->memberCount;
    roomData->joined = 1;  // broadcast only sent to members

    // Update lobby room card
    int infoRoomNumber = roomData->roomNumber;
    m_pFriendList->refreshRoomItemInLobby(info->roomId, infoRoomNumber, roomName, info->memberCount, 1);

    // Show system message in room chat if viewing this room
    if (m_pFriendList->getCurrentRoomId() == info->roomId) {
        if (info->memberCount > oldCount) {
            m_pFriendList->displayRoomMessage("系统", "有新成员加入了会议");
        } else if (info->memberCount < oldCount) {
            m_pFriendList->displayRoomMessage("系统", "有成员离开了会议");
        }
    }
}

// ========== Audio/Video Handlers ==========

void CKernel::dealAudioFrame(char *data, int len, unsigned long from)
{
    (void)from;
    if (!m_pAudioWrite) return;
    PROT_AUDIO_FRAME* frame = (PROT_AUDIO_FRAME*)data;
    if (frame->frameSize <= 0 || frame->frameSize > DEF_AUDIO_FRAME_SIZE) return;
    QByteArray ba(frame->data, frame->frameSize);

    if (!m_pAudioDecoder) {
        m_pAudioDecoder = new AudioDecoder(this);
        connect(m_pAudioDecoder, &AudioDecoder::sig_decodedFrame,
                m_pAudioWrite, &Audio_Write::slot_net_rx);
    }
    m_pAudioDecoder->decode(ba);
}

void CKernel::dealVideoFrame(char *data, int len, unsigned long from)
{
    (void)from;
    PROT_VIDEO_FRAME* frame = (PROT_VIDEO_FRAME*)data;
    qDebug() << "[DEBUG] dealVideoFrame userId:" << frame->userId << "frameSize:" << frame->frameSize;
    if (frame->frameSize < 0 || frame->frameSize > DEF_VIDEO_FRAME_SIZE) return;
    if (frame->frameSize == 0) {
        qDebug() << "[DEBUG] dealVideoFrame CLEAR userId:" << frame->userId;
        m_pFriendList->clearVideoFrame(frame->userId);
    } else {
        QByteArray encoded(frame->data, frame->frameSize);

        if (!m_pVideoDecoder) {
            m_pVideoDecoder = new VideoDecoder(this);
            connect(m_pVideoDecoder, &VideoDecoder::sig_decodedFrame, this,
                [this](QImage img, int userId) {
                    QByteArray jpeg;
                    QBuffer buf(&jpeg);
                    buf.open(QIODevice::WriteOnly);
                    img.save(&buf, "JPEG", 70);
                    m_pFriendList->displayVideoFrame(userId, jpeg);
                });
        }
        m_pVideoDecoder->decode(encoded, frame->userId);
    }
}

// ========== Room Slots ==========

void CKernel::slots_createRoom(QString roomName, QString roomPass)
{
    qDebug() << __func__;
    // 打包
    PROT_ROOM_CREATE_RQ rq;
    rq.userId = m_id;
    Utf8Togb2312(roomName, rq.roomName, sizeof(rq.roomName));
    Utf8Togb2312(roomPass, rq.roomPass, sizeof(rq.roomPass));
    // 发给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_joinRoom(int roomId, QString roomPass)
{
    qDebug() << __func__ << "roomId:" << roomId << "roomPass:" << roomPass << "m_id:" << m_id;
    // 打包
    PROT_ROOM_JOIN_RQ rq;
    rq.userId = m_id;
    rq.roomId = roomId;
    Utf8Togb2312(roomPass, rq.roomPass, sizeof(rq.roomPass));
    // 发给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_leaveRoom(int roomId)
{
    qDebug() << __func__;
    // 打包
    PROT_ROOM_LEAVE_RQ rq;
    rq.userId = m_id;
    rq.roomId = roomId;
    // 发给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_endMeeting(int roomId)
{
    qDebug() << __func__ << roomId;
    PROT_ROOM_END_RQ rq;
    rq.userId = m_id;
    rq.roomId = roomId;
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_copyRoomNumber()
{
    qDebug() << __func__;
    // Placeholder for future room number copy feature
}

void CKernel::slots_roomChatMessage(int roomId, QString content)
{
    qDebug() << __func__;
    // 打包
    PROT_ROOM_CHAT_RQ rq;
    rq.userId = m_id;
    rq.roomId = roomId;
    // Safe copy with truncation (P2-1 fix)
    QByteArray bytes = content.toUtf8();
    size_t copyLen = qMin((size_t)bytes.size(), sizeof(rq.content) - 1);
    memcpy(rq.content, bytes.constData(), copyLen);
    rq.content[copyLen] = '\0';
    // 发给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_roomListRequest()
{
    qDebug() << __func__;
    // 打包
    PROT_ROOM_LIST_RQ rq;
    rq.userId = m_id;
    // Clear existing room list items in lobby
    m_pFriendList->clearRoomList();
    // 发给服务端
    m_pMediator->sendData((char*)&rq, sizeof(rq), 89);
}

void CKernel::slots_showRoomCreate()
{
    qDebug() << __func__;
    RoomCreate* dlg = new RoomCreate(m_pFriendList);
    connect(dlg, &RoomCreate::sig_createRoom,
            this, &CKernel::slots_createRoom);
    dlg->exec();
    delete dlg;
}

void CKernel::slots_showRoomJoin()
{
    qDebug() << __func__;

    // Build a proper dialog instead of cramped QInputDialog
    QDialog dlg(m_pFriendList);
    dlg.setWindowTitle("输入房间号加入会议");
    dlg.setFixedSize(380, 260);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(24, 20, 24, 20);

    QLabel* lb_id = new QLabel("房间号", &dlg);
    lb_id->setFont(QFont("Microsoft YaHei", 14));
    lb_id->setStyleSheet("color: #1E293B; background: transparent;");

    QLineEdit* le_id = new QLineEdit(&dlg);
    le_id->setMinimumHeight(48);
    le_id->setFont(QFont("Microsoft YaHei", 15));
    le_id->setPlaceholderText("请输入8位房间号");
    le_id->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px; padding: 10px 14px;"
        "  background-color: #FFFFFF; font-size: 15px; color: #1E293B;"
        "}"
        "QLineEdit:focus { border: 1px solid #1677FF; background-color: #FFFFFF; }");
    le_id->setMaxLength(8);
    le_id->setValidator(new QIntValidator(10000000, 99999999, le_id));

    QLabel* lb_pass = new QLabel("房间密码（可选）", &dlg);
    lb_pass->setFont(QFont("Microsoft YaHei", 14));
    lb_pass->setStyleSheet("color: #1E293B; background: transparent;");

    QLineEdit* le_pass = new QLineEdit(&dlg);
    le_pass->setMinimumHeight(48);
    le_pass->setFont(QFont("Microsoft YaHei", 15));
    le_pass->setEchoMode(QLineEdit::Password);
    le_pass->setPlaceholderText("无密码则留空");
    le_pass->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px; padding: 10px 14px;"
        "  background-color: #FFFFFF; font-size: 15px; color: #1E293B;"
        "}"
        "QLineEdit:focus { border: 1px solid #1677FF; background-color: #FFFFFF; }");

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    btnLayout->addSpacerItem(spacer);

    QPushButton* pb_cancel = new QPushButton("取消", &dlg);
    pb_cancel->setMinimumSize(80, 38);
    pb_cancel->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF; color: #1677FF;"
        "  border: 1px solid #1677FF; border-radius: 8px;"
        "  padding: 8px 20px; font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #F0F5FF; }");
    QObject::connect(pb_cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnLayout->addWidget(pb_cancel);

    QPushButton* pb_join = new QPushButton("加入", &dlg);
    pb_join->setMinimumSize(80, 38);
    pb_join->setStyleSheet(
        "QPushButton {"
        "  background-color: #1677FF; color: #FFFFFF;"
        "  border: none; border-radius: 8px;"
        "  padding: 8px 24px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #3D8FFF; }");
    pb_join->setDefault(true);
    btnLayout->addWidget(pb_join);

    layout->addWidget(lb_id);
    layout->addWidget(le_id);
    layout->addWidget(lb_pass);
    layout->addWidget(le_pass);
    layout->addSpacing(8);
    layout->addLayout(btnLayout);

    // Connect join button
    QObject::connect(pb_join, &QPushButton::clicked, [&]() {
        QString idStr = le_id->text().trimmed();
        if (idStr.isEmpty()) {
            QMessageBox::about(&dlg, "提示", "请输入房间号");
            return;
        }
        if (idStr.length() != 8) {
            QMessageBox::about(&dlg, "提示", "房间号须为八位，请重新输入");
            return;
        }
        bool ok;
        int roomId = idStr.toInt(&ok);
        if (!ok || roomId <= 0) {
            QMessageBox::about(&dlg, "提示", "请输入有效的房间号");
            return;
        }
        dlg.accept();
    });

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QString roomIdStr = le_id->text().trimmed();
    int roomId = roomIdStr.toInt();
    QString roomPass = le_pass->text().trimmed();
    slots_joinRoom(roomId, roomPass);
}

void CKernel::slots_showRoomJoinById(int roomId)
{
    qDebug() << __func__ << roomId;

    // Simplified join dialog with roomId pre-filled
    QDialog dlg(m_pFriendList);
    dlg.setWindowTitle("加入会议");
    dlg.setFixedSize(380, 200);

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(24, 20, 24, 20);

    QLabel* lb_info = new QLabel(QString("房间号: %1").arg(roomId), &dlg);
    lb_info->setFont(QFont("Microsoft YaHei", 15));
    lb_info->setStyleSheet("color: #1677FF; background: transparent; font-weight: bold;");

    QLabel* lb_pass = new QLabel("房间密码（可选）", &dlg);
    lb_pass->setFont(QFont("Microsoft YaHei", 14));
    lb_pass->setStyleSheet("color: #1E293B; background: transparent;");

    QLineEdit* le_pass = new QLineEdit(&dlg);
    le_pass->setMinimumHeight(48);
    le_pass->setFont(QFont("Microsoft YaHei", 15));
    le_pass->setEchoMode(QLineEdit::Password);
    le_pass->setPlaceholderText("无密码则留空");
    le_pass->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 8px; padding: 10px 14px;"
        "  background-color: #FFFFFF; font-size: 15px; color: #1E293B;"
        "}"
        "QLineEdit:focus { border: 1px solid #1677FF; background-color: #FFFFFF; }");
    le_pass->setMaxLength(20);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    btnLayout->addSpacerItem(spacer);

    QPushButton* pb_cancel = new QPushButton("取消", &dlg);
    pb_cancel->setMinimumSize(80, 38);
    pb_cancel->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF; color: #1677FF;"
        "  border: 1px solid #1677FF; border-radius: 8px;"
        "  padding: 8px 20px; font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #F0F5FF; }");
    QObject::connect(pb_cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnLayout->addWidget(pb_cancel);

    QPushButton* pb_join = new QPushButton("加入", &dlg);
    pb_join->setMinimumSize(80, 38);
    pb_join->setStyleSheet(
        "QPushButton {"
        "  background-color: #1677FF; color: #FFFFFF;"
        "  border: none; border-radius: 8px;"
        "  padding: 8px 24px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #3D8FFF; }");
    pb_join->setDefault(true);
    btnLayout->addWidget(pb_join);

    layout->addWidget(lb_info);
    layout->addWidget(lb_pass);
    layout->addWidget(le_pass);
    layout->addSpacing(8);
    layout->addLayout(btnLayout);

    QObject::connect(pb_join, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QString roomPass = le_pass->text().trimmed();
    slots_joinRoom(roomId, roomPass);
}

void CKernel::slots_removeRoomFromList(int roomId)
{
    qDebug() << __func__ << roomId;
    // If still a member, leave the room on server first
    if (m_mapRoomData.contains(roomId) && m_mapRoomData[roomId]->joined) {
        qDebug() << __func__ << "leaving room before removing from list";
        slots_leaveRoom(roomId);
    }
    m_setRemovedRooms.insert(roomId);
    m_setEverJoinedRooms.remove(roomId);
    if (m_mapRoomData.contains(roomId)) {
        delete m_mapRoomData[roomId];
        m_mapRoomData.remove(roomId);
    }
}

void CKernel::slots_enterRoom(int roomId)
{
    qDebug() << __func__ << roomId;

    // Re-join room if not currently a member (e.g. creator left and re-enters)
    // Server handles this idempotently — no-op if already a member
    slots_joinRoom(roomId, "");

    // Look up room name and number from local data
    QString roomName = QString("房间%1").arg(roomId);
    int roomNumber = 0;
    if (m_mapRoomData.contains(roomId)) {
        roomName = m_mapRoomData[roomId]->roomName;
        roomNumber = m_mapRoomData[roomId]->roomNumber;
    }

    // Switch to embedded room view
    bool isRoomCreator = m_setCreatedRooms.contains(roomId);
    qDebug() << "[DEBUG slots_enterRoom] roomId:" << roomId << "m_setCreatedRooms:" << m_setCreatedRooms << "isRoomCreator:" << isRoomCreator;
    m_pFriendList->showRoomView(roomId, roomNumber, roomName, isRoomCreator);

    // Flush any pending messages cached for this room
    if (m_mapPendingRoomMessages.contains(roomId)) {
        QStringList& pending = m_mapPendingRoomMessages[roomId];
        for (const QString& msg : pending) {
            int tabIdx = msg.indexOf('\t');
            if (tabIdx > 0) {
                QString senderName = msg.left(tabIdx);
                QString content = msg.mid(tabIdx + 1);
                m_pFriendList->displayRoomMessage(senderName, content);
            }
        }
        m_mapPendingRoomMessages.remove(roomId);
    }
}

// ========== Audio/Video Slots ==========

void CKernel::slots_startRoomMedia(int roomId)
{
    qDebug() << "[DEBUG] slots_startRoomMedia roomId:" << roomId << "mediaRoomId:" << m_mediaRoomId;
    if (m_mediaRoomId != 0) { qDebug() << "[DEBUG] slots_startRoomMedia SKIP (already in room)"; return; }
    m_mediaRoomId = roomId;

    if (!m_pAudioRead) {
        qDebug() << "[DEBUG] creating Audio_Read";
        m_pAudioRead = new Audio_Read(this);
    }
    if (!m_pAudioEncoder) {
        m_pAudioEncoder = new AudioEncoder(this);
        connect(m_pAudioEncoder, &AudioEncoder::sig_encodedFrame,
                this, &CKernel::slots_sendAudioFrame);
    }
    // Re-route: Audio_Read → AudioEncoder → send
    disconnect(m_pAudioRead, &Audio_Read::SIG_audioFrame, this, &CKernel::slots_sendAudioFrame);
    connect(m_pAudioRead, &Audio_Read::SIG_audioFrame,
            m_pAudioEncoder, &AudioEncoder::encode);
    if (!m_pAudioWrite) {
        qDebug() << "[DEBUG] creating Audio_Write";
        m_pAudioWrite = new Audio_Write(this);
    }
    if (!m_pVideoCapture) {
        qDebug() << "[DEBUG] creating VideoCapture";
        m_pVideoCapture = new VideoCapture(this);
        // Keep JPEG path for local preview
        connect(m_pVideoCapture, &VideoCapture::sig_videoFrameReady, this, [this](QByteArray ba) {
            if (!m_mediaRoomId || ba.isEmpty()) return;
            slots_sendVideoFrame(ba);
        });
    }
    if (!m_pVideoEncoder) {
        m_pVideoEncoder = new VideoEncoder(this);
        // H.264 encoded frames → network only (not JPEG, can't display locally)
        connect(m_pVideoEncoder, &VideoEncoder::sig_encodedFrame, this, [this](QByteArray ba) {
            if (!m_mediaRoomId || ba.isEmpty()) return;
            slots_sendVideoFrame(ba, false, false);
        });
    }
    // Route raw frames through H.264 encoder → send
    connect(m_pVideoCapture, &VideoCapture::sig_rawFrameReady,
            m_pVideoEncoder, &VideoEncoder::encode);
    if (!m_pScreenCapture) {
        qDebug() << "[DEBUG] creating ScreenCapture";
        m_pScreenCapture = new ScreenCapture(this);
        // Screen frames → display locally as screen (isScreen=true)
        connect(m_pScreenCapture, &ScreenCapture::sig_screenFrameReady, this, [this](QByteArray ba) {
            if (!m_mediaRoomId || ba.isEmpty()) return;
            slots_sendVideoFrame(ba, true, true);
        });
    }
    qDebug() << "[DEBUG] slots_startRoomMedia DONE";
}

void CKernel::slots_stopRoomMedia()
{
    qDebug() << __func__;
    m_mediaRoomId = 0;
    if (m_pAudioRead) m_pAudioRead->pause();
    if (m_pVideoCapture) m_pVideoCapture->stop();
    if (m_pScreenCapture) m_pScreenCapture->stop();
}

void CKernel::slots_sendAudioFrame(QByteArray ba)
{
    if (m_mediaRoomId == 0) return;
    if (ba.size() <= 0 || ba.size() > DEF_AUDIO_FRAME_SIZE) return;
    PROT_AUDIO_FRAME frame;
    frame.userId = m_id;
    frame.roomId = m_mediaRoomId;
    frame.frameSize = ba.size();
    memcpy(frame.data, ba.constData(), ba.size());
    m_pMediator->sendData((char*)&frame, sizeof(frame), 89);
}

void CKernel::slots_sendVideoFrame(QByteArray ba, bool displayLocally, bool isScreen)
{
    if (m_mediaRoomId == 0) return;
    if (ba.size() > DEF_VIDEO_FRAME_SIZE) return;
    if (ba.isEmpty()) {
        // Empty frame = "clear" signal
        PROT_VIDEO_FRAME frame;
        frame.userId = m_id;
        frame.roomId = m_mediaRoomId;
        frame.frameSize = 0;
        memset(frame.data, 0, DEF_VIDEO_FRAME_SIZE);
        m_pMediator->sendData((char*)&frame, sizeof(frame), 89);
    } else {
        if (displayLocally)
            m_pFriendList->displayVideoFrame(m_id, ba, isScreen);
        PROT_VIDEO_FRAME frame;
        frame.userId = m_id;
        frame.roomId = m_mediaRoomId;
        frame.frameSize = ba.size();
        memcpy(frame.data, ba.constData(), ba.size());
        m_pMediator->sendData((char*)&frame, sizeof(frame), 89);
    }
}

void CKernel::slots_toggleMute(bool muted)
{
    if (muted) { if (m_pAudioRead) m_pAudioRead->pause(); }
    else       { if (m_pAudioRead) m_pAudioRead->start(); }
}

void CKernel::slots_toggleCamera(bool off)
{
    qDebug() << "[DEBUG] slots_toggleCamera off:" << off << "hasCamera:" << (m_pVideoCapture != nullptr) << "mediaRoom:" << m_mediaRoomId;
    if (off) {
        if (m_pVideoCapture) { m_pVideoCapture->stop(); qDebug() << "[DEBUG] Camera stopped"; }
        m_pFriendList->clearVideoFrame(m_id);
        if (!m_screenSharing)  // don't clear screen with camera-off signal
            slots_sendVideoFrame(QByteArray());
    } else {
        if (m_pVideoCapture) {
            m_pVideoCapture->start();
            qDebug() << "[DEBUG] Camera start() called";
        } else {
            qDebug() << "[DEBUG] Camera is NULL!";
        }
    }
}

void CKernel::slots_toggleScreenShare()
{
    qDebug() << "[DEBUG] slots_toggleScreenShare was:" << m_screenSharing << "hasScreen:" << (m_pScreenCapture != nullptr);
    m_screenSharing = !m_screenSharing;
    qDebug() << "[DEBUG] slots_toggleScreenShare now:" << m_screenSharing;
    if (m_screenSharing) {
        if (m_pScreenCapture) { m_pScreenCapture->start(); qDebug() << "[DEBUG] ScreenCapture started"; }
        else qDebug() << "[DEBUG] ScreenCapture is NULL!";
    } else {
        if (m_pScreenCapture) m_pScreenCapture->stop();
        m_pFriendList->clearVideoFrame(m_id);
        slots_sendVideoFrame(QByteArray());
    }
}

