#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QCloseEvent>
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

signals:
    //用来把注册数据发给Kernel
    void sig_registerData(QString name,QString tel,QString pass);
    //用来把登录数据发给Kernel
    void sig_loginData(QString tel,QString pass);
    //通知kernel，关闭程序
    void sig_closeProcess();
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    //重写关闭事件
    void closeEvent(QCloseEvent * event);
private slots:
    void on_pb_register_clicked();

    void on_pb_login_clicked();

    void on_pb_login_clear_clicked();

    void on_pb_clear_clicked();



private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
