#include "INet.h"
#include "def.h"
#include <process.h>
#include <atomic>
#include<iostream>
using namespace std;
class TCPClient :public INet {
public:
    TCPClient(INetMediator* p);
	~TCPClient();

	//初始化网络
	bool initNet();
	//发送数据(udp:ip（ulong类型）决定发给谁，tcp里socket （uint类型）决定发给谁)
	//UDP：sendto(socket,buf,len,flag,to,tolen);
	//TCP：send(socket,buf,len,flag);  sock都有
	bool sendData(char* data, int len, unsigned long to/*用长的那个，能在udp装ip，能在tcp里传socket*/);
	//接受数据(再创一个线程做等待)
	void recvData();
	//关闭网络(回收接受线程的资源，关闭套接字，卸载库)
	void unInitNet();
	//接受数据的线程函数
	static unsigned __stdcall recvThread(void* lpVoid);
private:
	HANDLE m_handle;
	std::atomic<bool> run{true};
};
