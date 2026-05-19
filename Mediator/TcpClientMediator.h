#pragma once
#include"INetMediator.h"

class TcpClientMediator :public INetMediator {
public:
	TcpClientMediator();
	~TcpClientMediator();
	//打开网络
	bool openNet();
	//关闭网络
	void closeNet();
	//发送数据
	bool sendData(char* data, int len, unsigned long to);
	//转发数据
	void transmateData(char* data, int len, unsigned long from);
};