#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Winsock2.h>
#include"../Mediator/INetmediator.h"
//#pragma comment(lib,"Ws2_32.lib")
class INet {
public:
	INet():m_sock(INVALID_SOCKET),m_pMediator(nullptr) {}
	virtual ~INet() {}
	//初始化网络
	virtual bool initNet() = 0;
	//发送数据(udp:ip（ulong类型）决定发给谁，tcp里socket （uint类型）决定发给谁)
	//UDP：sendto(socket,buf,len,flag,to,tolen);
	//TCP：sendto(socket,buf,len,flag);  sock都有
	virtual bool sendData(char* data, int len, unsigned long to/*用长的那个，能在udp装ip，能在tcp里传socket*/) = 0;
	//接受数据(再创一个线程做等待)
	virtual void recvData() = 0;
	//关闭网络(回收接受线程的资源，关闭套接字，卸载库)
	virtual void unInitNet() = 0;
protected:
	SOCKET m_sock;
	INetMediator* m_pMediator;
};
