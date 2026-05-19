#include "frienditem.h"
#include "ui_frienditem.h"
#include<QDebug>
frienditem::frienditem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::frienditem)
{
    ui->setupUi(this);
}

frienditem::~frienditem()
{
    delete ui;
}

void frienditem::setFriendInfo(int friendId, QString name, QString feeling, int icon_id, int status)
{
    //保存到成员变量中
    m_friendId=friendId;
    m_name=name;
    m_feeling=feeling;
    m_icon_id=icon_id;
    m_status=status;

    //设置昵称和签名
    ui->lb_name->setText(m_name);
    ui->lb_feeling->setText(m_feeling);

    //彩色圆形头像 (基于icon_id取色)
    QStringList colors;
    colors << "#FF6B6B" << "#4ECDC4" << "#45B7D1" << "#96CEB4" << "#FFEAA7"
           << "#DDA0DD" << "#98D8C8" << "#F7DC6F" << "#BB8FCE" << "#85C1E9"
           << "#F8C471" << "#82E0AA" << "#F1948A" << "#73C6B6" << "#D2B4DE"
           << "#E59866" << "#AED6F1" << "#FAD7A0" << "#ABEBC6" << "#C39BD3";
    QString bgColor = colors[icon_id % colors.size()];
    QString initial = m_name.isEmpty() ? QString("?") : QString(m_name.at(0));

    if(DEF_STATUS_ONLINE==status)
    {
        ui->pb_icon->setStyleSheet(
            QString("QPushButton#pb_icon {"
                    "background-color: %1;"
                    "color: #FFFFFF;"
                    "border: 2px solid %1;"
                    "border-radius: 30px;"
                    "font-size: 22px;"
                    "font-weight: bold;"
                    "}").arg(bgColor));
    }else{
        ui->pb_icon->setStyleSheet(
            QString("QPushButton#pb_icon {"
                    "background-color: #BDBDBD;"
                    "color: #FFFFFF;"
                    "border: 2px solid #BDBDBD;"
                    "border-radius: 30px;"
                    "font-size: 22px;"
                    "font-weight: bold;"
                    "}"));
    }
    ui->pb_icon->setText(initial);
}

void frienditem::setFriendOffline()
{

    qDebug()<<__func__;
    m_status=DEF_STATUS_OFFLINE;

    QString initial = m_name.isEmpty() ? QString("?") : QString(m_name.at(0));
    ui->pb_icon->setStyleSheet(
        QString("QPushButton#pb_icon {"
                "background-color: #BDBDBD;"
                "color: #FFFFFF;"
                "border: 2px solid #BDBDBD;"
                "border-radius: 30px;"
                "font-size: 22px;"
                "font-weight: bold;"
                "}"));
    ui->pb_icon->setText(initial);
    //重绘
    repaint();

}

void frienditem::on_pb_icon_clicked()
{
    Q_EMIT sig_showChatDialog(m_friendId);
}

const QString &frienditem::name() const
{
    return m_name;
}

