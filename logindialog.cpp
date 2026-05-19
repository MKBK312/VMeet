#include "logindialog.h"
#include "ui_logindialog.h"
#include<QDebug>
#include<QMessageBox>
LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    Q_EMIT sig_closeProcess();
}

void LoginDialog::on_pb_register_clicked()
{
    //用户点击了注册界面提交按钮，走到这个函数
    //1.从控件上获取用户输入的数据
    QString name=ui->le_register_name->text();
    QString tel=ui->le_register_tel->text();
    QString pass=ui->le_register_pass->text();
    QString nameTmp=name;
    QString telTmp=tel;
    QString passTmp=pass;
    qDebug()<<"name:"<<name<<" tel:"<<tel<< "pass:"<<pass;
    //2.校验数据的合法性
    //2.1是否为空或者全空格
    if(name.isEmpty()||tel.isEmpty()||pass.isEmpty()||
            nameTmp.remove(" ").isEmpty()||telTmp.remove(" ").isEmpty()||
            passTmp.remove(" ").isEmpty())    {
        QMessageBox::about(this,"提示","输入不能是空或全空格");
    }
    //2.2检查字符串长度是否合法
    if(name.length()>20||tel.length()!=11||pass.length()>20)
    {
        QMessageBox::about(this,"提示","昵称不超过20，电话号码必须是11，密码不超过20");
        return;
    }
    //2.3内容是否合法（昵称和密码只能是字母、数字、下划线、电话号码必须是全数字）


    //3.把数据发给Kernel
    sig_registerData(name,tel,pass);
}

void LoginDialog::on_pb_clear_clicked()
{
    ui->le_register_tel->setText("");
    ui->le_register_name->setText("");
    ui->le_register_pass->setText("");
    ui->le_register_pass2->setText("");
}

void LoginDialog::on_pb_login_clicked()
{

    //用户点击了登录界面登录按钮，走到这个函数
    //1.从控件上获取用户输入的数据
    QString tel=ui->le_tel->text();
    QString pass=ui->le_pass->text();
    QString telTmp=tel;
    QString passTmp=pass;
    //2.校验数据的合法性
    //2.1是否为空或者全空格
    if(tel.isEmpty()||pass.isEmpty()||
           telTmp.remove(" ").isEmpty()||
            passTmp.remove(" ").isEmpty())    {
        QMessageBox::about(this,"提示","输入不能是空或全空格");
    }
    //2.2检查字符串长度是否合法
    if(tel.length()!=11||pass.length()>20)
    {
        QMessageBox::about(this,"提示","电话号码必须是11，密码不超过20");
        return;
    }
    //2.3内容是否合法（昵称和密码只能是字母、数字、下划线、电话号码必须是全数字）


    //3.把数据发给Kernel
    sig_loginData(tel,pass);
}


void LoginDialog::on_pb_login_clear_clicked()
{
 ui->le_pass->setText("");
 ui->le_tel->setText("");
}



