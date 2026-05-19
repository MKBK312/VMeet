#include"ckernel.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    qDebug() << "main: before QApplication";
    QApplication a(argc, argv);
    qDebug() << "main: after QApplication";

    // 加载全局扁平样式表
    QFile qssFile(":/style.qss");
    qDebug() << "main: QSS file open=" << qssFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (qssFile.isOpen()) {
        QTextStream stream(&qssFile);
        QString qss = stream.readAll();
        qDebug() << "main: QSS size=" << qss.size();
        a.setStyleSheet(qss);
        qssFile.close();
        qDebug() << "main: QSS loaded";
    }

    qDebug() << "main: before CKernel";
    CKernel kernel;
    qDebug() << "main: after CKernel";
    return a.exec();
}

//此前在vs使用一个指针指向对象调用Server对象
//QT信号和槽机制（特有）信号相当于红绿灯，槽相当于道路
//两个类之间通知或传输数据
//1、两个类必须直接或间接继承子QObject
//2、发送数据的类的头文件中声明signals信号，返回值只能是void，参数就是要传输的数据，也可以没有参数（单纯通知事情发生）
//3、信号不是函数，不需要再cpp中实现，需要在通知或发送数据的地方使用Q_EMIT/qemit 信号名（参数列表）发送信号
//4、要在接受数据的类中使用public：/protected:/private:/slots声明槽函数,槽函数的参数类型和返回值要和信号一致
//槽函数需要在cpp中实现
//4、在接受数据得类中，发送数据的对象new出来后，绑定信号和槽函数

// 字符串可以存在char*、QString、std::string
// char*可以直接给QString和std::string赋值，因为类里面重载了等号操作符
// std::string.c_str()=>char*
// QString.toStdString()=>std::string.c_str()=>char*

//QT是用的是utf-8的编码方式，vs是采用的gb2312的编码方式
//统一在客户端转码
//QT先转成gb2312，再发给vs
//QT接收到的数据，先转成utf-8，再设置到界面上
//UTF-8编码方式的字符串保存在QString里
//gb2312编码方式的字符串保存在char*里
