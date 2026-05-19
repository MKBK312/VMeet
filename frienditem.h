#ifndef FRIENDITEM_H
#define FRIENDITEM_H
#include"./net/def.h"
#include <QWidget>
#include<QMessageBox>
namespace Ui {
class frienditem;
}

class frienditem : public QWidget
{
    Q_OBJECT
signals:
    void sig_showChatDialog(int friendId);
public:
    explicit frienditem(QWidget *parent = nullptr);
    ~frienditem();
    //设置好友信息
    void setFriendInfo(int friendId,QString name,QString m_feeling,int icon_id,int status);
    //设置好友下线
    void setFriendOffline();
    const QString &name() const;

private slots:
    void on_pb_icon_clicked();

private:
    Ui::frienditem *ui;

    int m_friendId;
    QString m_name;
    QString m_feeling;
    int m_icon_id;
    int m_status;


};

#endif // FRIENDITEM_H
