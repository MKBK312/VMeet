#ifndef ROOMCREATE_H
#define ROOMCREATE_H

#include <QDialog>

namespace Ui {
class RoomCreate;
}

class RoomCreate : public QDialog
{
    Q_OBJECT
signals:
    void sig_createRoom(QString roomName, QString roomPass);
public:
    explicit RoomCreate(QWidget *parent = nullptr);
    ~RoomCreate();

private slots:
    void on_pb_ok_clicked();
    void on_pb_cancel_clicked();

private:
    Ui::RoomCreate *ui;
};

#endif // ROOMCREATE_H
