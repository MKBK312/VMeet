#pragma once
#include<QObject>
//先声明，后使用，先用着，后面就编译到了（前提是类存在）
class INet;
class INetMediator:public QObject {
    Q_OBJECT
public:
    INetMediator();
    virtual ~INetMediator();
	//打开网络
	virtual bool openNet() = 0;
	//关闭网络
	virtual void closeNet() = 0;
	//发送数据
	virtual bool sendData(char* data, int len, unsigned long to) = 0;
	//转发数据  把net层收到的数据发给核心处理类
	virtual void transmateData(char* data, int len, unsigned long from) = 0;
signals:
    //把接收到的数据发送给核心处理类
    void sig_dealData(char* data, int len, unsigned long from);

protected:
	INet* m_pNet;
};
