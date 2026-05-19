#ifndef ROOMCHAT_H
#define ROOMCHAT_H

#include <QDialog>

namespace Ui {
class RoomChat;
}

class RoomChat : public QDialog
{
    Q_OBJECT
signals:
    void sig_roomChatMessage(int roomId, QString content);
public:
    explicit RoomChat(QWidget *parent = nullptr);
    ~RoomChat();

    void setRoomInfo(int roomId, QString roomName);
    void setChatMessage(QString senderName, QString content);

private slots:
    void on_pb_send_clicked();

private:
    Ui::RoomChat *ui;
    int m_roomId;
    QString m_roomName;
};

#endif // ROOMCHAT_H
