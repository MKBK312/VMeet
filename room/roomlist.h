#ifndef ROOMLIST_H
#define ROOMLIST_H

#include <QDialog>
#include <QVBoxLayout>
#include <QMap>
#include <QLabel>
#include <QPushButton>

class RoomItem;

namespace Ui {
class RoomList;
}

class RoomList : public QDialog
{
    Q_OBJECT
signals:
    void sig_createRoom();
    void sig_joinRoom();
    void sig_selectRoom(int roomId);
    void sig_refreshRoomList();

public:
    explicit RoomList(QWidget *parent = nullptr);
    ~RoomList();

    void addRoomItem(int roomId, QString roomName, int memberCount);
    void clearRoomList();

private slots:
    void on_pb_create_clicked();
    void on_pb_join_clicked();
    void on_pb_refresh_clicked();
    void on_pb_close_clicked();

private:
    Ui::RoomList *ui;
    QVBoxLayout* m_playout;
    QMap<int, QWidget*> m_mapRoomItems;
};

// Room item widget (like frienditem for rooms)
class RoomItem : public QWidget
{
    Q_OBJECT
signals:
    void sig_clicked(int roomId);
public:
    explicit RoomItem(QWidget *parent = nullptr);
    void setRoomInfo(int roomId, QString roomName, int memberCount);

private:
    int m_roomId;
    QString m_roomName;
    QPushButton* m_pb_icon;
    QLabel* m_lb_name;
    QLabel* m_lb_count;
};

#endif // ROOMLIST_H
