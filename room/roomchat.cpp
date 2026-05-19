#include "roomchat.h"
#include "ui_roomchat.h"
#include <QDebug>
#include <QTime>

RoomChat::RoomChat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomChat)
{
    ui->setupUi(this);
}

RoomChat::~RoomChat()
{
    delete ui;
}

void RoomChat::setRoomInfo(int roomId, QString roomName)
{
    m_roomId = roomId;
    m_roomName = roomName;
    setWindowTitle(QString("群聊 - %1").arg(roomName));
}

void RoomChat::setChatMessage(QString senderName, QString content)
{
    ui->tb_chat->append(QString("【%1】 %2").arg(senderName).arg(QTime::currentTime().toString("hh:mm:ss")));
    ui->tb_chat->append(content);
}

void RoomChat::on_pb_send_clicked()
{
    qDebug() << __func__;
    // 1、获取用户输入文本（纯文本）
    QString content = ui->te_chat->toPlainText();
    // 2、校验内容是否为空或者全空格
    QString contentTmp = content;
    if (content.isEmpty() || contentTmp.remove(" ").isEmpty()) {
        ui->te_chat->setText("");
        return;
    }
    // 3、获取用户输入内容（带格式的）
    content = ui->te_chat->toHtml();
    qDebug() << content;

    // 4、把内容显示到上面的浏览窗口上
    ui->tb_chat->append(QString("我 %1").arg(QTime::currentTime().toString("hh:mm:ss")));
    ui->tb_chat->append(content);
    // 5、清空下面的编辑窗口
    ui->te_chat->setText("");
    // 6、把聊天内容发给kernel
    Q_EMIT sig_roomChatMessage(m_roomId, content);
}
