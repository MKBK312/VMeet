#include"UDP.h"
#include"../Mediator/UdpNetMediator.h"
Udp::Udp(INetMediator* p) :m_handle(nullptr), run(true)
{
	m_pMediator = p;
}

Udp::~Udp()
{
}

//接受数据的线程函数
unsigned __stdcall Udp::recvThread(void* lpVoid)
{
	Udp* pThis = (Udp*)lpVoid;
	pThis->recvData();
	return 1;
}

//初始化网络：加载库、创建套接字、绑定ip和端口
bool Udp::initNet()
{
	//1加载库
	//魔鬼数字：把数字定义成宏
	WORD version = MAKEWORD(_DEF_VERSION_HIGH, _DEF_VERSION_LOW);
	WSADATA data = {};
	int err = WSAStartup(version, &data);
	if (0 != err)
	{
		cout << "WSAStartup failed" << endl;
		return false;
	}
	if (_DEF_VERSION_HIGH == HIBYTE(data.wVersion) && _DEF_VERSION_LOW == LOBYTE(data.wVersion))
	{
		cout << "WSAStartup success" << endl;
	}
	else {
		cout << "WSAStartup failed!" << endl;
		return false;
	}

	//2创建套接字
	m_sock = socket(AF_INET,
		SOCK_DGRAM,
		IPPROTO_UDP);
	if (INVALID_SOCKET == m_sock)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "socket success" << endl;
	}

	//3绑定端口和ip
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_DEF_UDP_PORT);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	err = bind(m_sock, (sockaddr*)&addr, sizeof(sockaddr));
	if (SOCKET_ERROR == err)
	{
		cout << "bind error:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "bind success" << endl;
	}
	//4创建接受线程
	//CreateThread和ExitThread是一对（不用他）原因是如在C++运行时调用C++的运行时的库（e.g.strcpy）ExitThread在退出线程中不会回收申请的空间
	//内存泄漏

	//_beginthreadx和_endThreadx是一对，_endThreadx在退出线程时会先回收空间，再调用ExitThread

	//handle是一个操作线程的东西相当于线程句柄
	m_handle = (HANDLE)_beginthreadex(0/*安全级别*/,
		0/*默认堆栈大小*/,
		&recvThread/*线程执行的函数起始地址*/,
		this/*线程执行的函数*/,
		0/*初始化标志位，0是创建后立即挂起，CREATE_SUSPENDED是创建以后挂起*/,
		nullptr/*创建线程后，操作系统给每个线程分配的线程*/
	);

	return true;
}


bool Udp::sendData(char* data, int len, unsigned long to)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_DEF_UDP_PORT);
	addr.sin_addr.S_un.S_addr = to;
	int nSendNum = sendto(m_sock, data, len, 0, (sockaddr*)&addr, sizeof(addr));
	if (SOCKET_ERROR == nSendNum)
	{
		cout << "sendto error:" << WSAGetLastError() << endl;
		return false;
	}

	return true;
}

void Udp::recvData()
{
	cout << "UDP" << __func__ << endl;
	int nRecvNum = 0;
	char recvbuf[4096] = "";
	sockaddr_in addrFrom = {};
	int size = sizeof(addrFrom);
	while (run)
	{
		int nRecvNum = recvfrom(m_sock, recvbuf, sizeof(recvbuf), 0, (sockaddr*)&addrFrom, &size);
		if (nRecvNum > 0)
		{
			//接受一个数据包成功
			//根据数据大小申请空间
			char* pPack = new char[nRecvNum];
			//把接收到的数据拷贝到新的空间
			memcpy(pPack, recvbuf, nRecvNum);
			//把接收到的数据传给中介者
			m_pMediator->
				transmateData(pPack, nRecvNum, addrFrom.sin_addr.S_un.S_addr);
		}
		else {
			cout << "recv error:" << WSAGetLastError() << endl;
			break;
		}
	}
}

void Udp::unInitNet()
{
	//回收线程资源
	//三个内核资源，线程id，句柄，内核对象 
	//想要回收线程，就需要让引用计数器编程0，结束线程工作，关闭句柄
	run = false;
	//Waitforasingleobject返回值如果等于WAIT_TIMEOUT，说明等待的线程在等待时间结束后，仍在运行
	if (WAIT_TIMEOUT == WaitForSingleObject(m_handle, 1000))
	{
		//如果线程还在运行，强制杀死线程
		TerminateThread(m_handle, -1);
	}
	//关闭句柄
		CloseHandle(m_handle);
		m_handle = nullptr;
	//关闭套接字
	if (m_sock && INVALID_SOCKET != m_sock)
	{
		closesocket(m_sock);
	}

	//卸载库
	WSACleanup();

}
