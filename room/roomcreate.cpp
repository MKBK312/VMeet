#include "roomcreate.h"
#include "ui_roomcreate.h"
#include <QDebug>
#include <QMessageBox>

RoomCreate::RoomCreate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RoomCreate)
{
    ui->setupUi(this);
}

RoomCreate::~RoomCreate()
{
    delete ui;
}

void RoomCreate::on_pb_ok_clicked()
{
    qDebug() << __func__;
    QString roomName = ui->le_name->text().trimmed();
    QString roomPass = ui->le_pass->text().trimmed();

    if (roomName.isEmpty()) {
        QMessageBox::about(this, "提示", "请输入房间名称");
        return;
    }
    if (roomName.length() > 20) {
        QMessageBox::about(this, "提示", "房间名称不能超过20个字符");
        return;
    }

    Q_EMIT sig_createRoom(roomName, roomPass);
    this->accept();
}

void RoomCreate::on_pb_cancel_clicked()
{
    qDebug() << __func__;
    this->reject();
}
