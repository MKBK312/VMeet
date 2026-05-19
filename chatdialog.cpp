#include "chatdialog.h"
#include "ui_chatdialog.h"
#include <QDebug>
#include<QTime>
ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog)
{
    ui->setupUi(this);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::setFriendinfo(int id, QString name)
{
    m_id=id;
    m_name=name;
    setWindowTitle(QString("与【%1】的聊天窗口").arg(name));
}

void ChatDialog::setChatMessage(QString content)
{
    //把内容显示到上面的浏览窗口上
    ui->tb_chat->append(QString("【%1】 %2").arg(m_name).arg(QTime::currentTime().toString("hh:mm:ss")));
    qDebug()<<"3:"<<content;
    ui->tb_chat->append(content);
}

void ChatDialog::setFriOffline()
{
    ui->tb_chat->append(QString("【%1】 %2不在线").arg(m_name).arg(QTime::currentTime().toString("hh:mm:ss")));

}

void ChatDialog::on_pb_send_clicked()
{
    //1、获取用户输入文本（纯文本）
    QString content=ui->te_chat->toPlainText();
    //2、校验内容是否为空或者全空格
    if(content.isEmpty()||content.remove("").isEmpty()){
       ui->te_chat->setText("");
       return;
    }
    //3、获取用户输入内容（带格式的）
    content=ui->te_chat->toHtml();
    qDebug()<<content;

    //4、把内容显示到上面的浏览窗口上
    ui->tb_chat->append(QString("我 %1").arg(QTime::currentTime().toString("hh:mm:ss")));
    ui->tb_chat->append(content);
    //5、清空下面的编辑窗口
    ui->te_chat->setText("");
    //6、把聊天内容发给kernel
    Q_EMIT sig_chatMessage(m_id,content);

}

