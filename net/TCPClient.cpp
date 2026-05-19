#include"TCPClient.h"
#include"../Mediator/TcpClientMediator.h"

TCPClient::TCPClient(INetMediator* p) :m_handle(nullptr)
{
    m_pMediator=p;
}

TCPClient::~TCPClient()
{
}

unsigned __stdcall TCPClient::recvThread(void* lpVoid)
{
	TCPClient* pthis = (TCPClient*)lpVoid;
	pthis->recvData();
	return 1;
}

//加载库
//创建套接字
//绑定ip
//连接服务端

bool TCPClient::initNet()
{

	//加载库
	WORD verison = MAKEWORD(_DEF_VERSION_HIGH, _DEF_VERSION_LOW);
	WSADATA data = {};
	int err = WSAStartup(verison, &data);
	if (0 != err)
	{
		cout << "WSAStartup err:" << WSAGetLastError() << endl;
		return false;
	}
	if (_DEF_VERSION_HIGH == LOBYTE(data.wVersion) && _DEF_VERSION_LOW == HIBYTE(data.wVersion))
	{
		cout << "client WSAStartup success" << endl;
	}
	else {
		cout << "WSAStartup err:" << WSAGetLastError() << endl;
		return false;
	}
	//创建套接字
	m_sock = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP
	);
	if (INVALID_SOCKET == m_sock)
	{
		cout << "sock err:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "sock success" << endl;
	}
	//绑定ip
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(_DEF_TCP_PORT);
    addrServer.sin_addr.S_un.S_addr = inet_addr("192.168.134.128");
	//连接服务端
	err = connect(m_sock, (sockaddr*)&addrServer, sizeof(addrServer));
	if (SOCKET_ERROR == err)
	{
		cout << "connect err:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "connect success" << endl;
	}
	//创建接受线程函数
	run = true;  // reset run flag (may have been set false by unInitNet)
	m_handle = (HANDLE)_beginthreadex(nullptr, 0, &recvThread, this, 0, nullptr);
	return true;
}

bool TCPClient::sendData(char* data, int len, unsigned long to)
{
    cout << "TCPClient:" << __func__ << endl;
	//检验数据合法性
	if (nullptr == data || len <= 0)
	{
		cout << "data error" << endl;
		return false;
	}

	//先发包的长度
	int nSendNum = send(m_sock, (char*)&len, sizeof(int), 0);
	if (SOCKET_ERROR == nSendNum)
	{
		cout << "len err:" << WSAGetLastError() << endl;
		return false;
	}
	cout << "len right!" << endl;
	//再发包的内容
	nSendNum = send(m_sock, data, len, 0);
	if (SOCKET_ERROR == nSendNum)
	{
		cout << "send err:" << WSAGetLastError() << endl;
		return false;
	}
	cout << "send success" << endl;

	return true;
}

void TCPClient::recvData()
{
	cout << "TCPClient:" << __func__ << endl;
	int nRecvNum = 0;
	//先接收包长度
	int nPackLen = 0;
	//记录一个包中有多少数据
	int nOffset = 0;
	while (run)
	{
		nRecvNum = recv(m_sock, (char*)&nPackLen, sizeof(int), 0);
		if (nRecvNum > 0) {
			//校验包长度的合法性
			if (nPackLen <= 0 || nPackLen > 1024 * 1024) {
				cout << "invalid nPackLen:" << nPackLen << endl;
				break;
			}
			//接受包长度以后再接受包内容
			//按照包长度new一个空间
			char* packBuf = new char[nPackLen];
			while (nPackLen > 0) {
				nRecvNum = recv(m_sock, packBuf + nOffset, nPackLen, 0);
					if (nRecvNum > 0)
					{
						nOffset += nRecvNum;
						nPackLen -= nRecvNum;
					}
					else {
						cout << "recv err2" << WSAGetLastError() << endl;
						break;
					}
            }
            m_pMediator->transmateData(packBuf,nOffset,m_sock);
		//每一个包接收完成后offset清零
			nOffset = 0;

		}
		else {
            cout << " client recv len error1" <<WSAGetLastError()<< endl;
			break;
		}
	}

}

void TCPClient::unInitNet()
{
	//先关闭套接字，中断阻塞的 recv()
	run = false;
	if (m_sock && INVALID_SOCKET != m_sock)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	//等待接收线程自然退出
	if (m_handle)
	{
		WaitForSingleObject(m_handle, 3000);
		CloseHandle(m_handle);
		m_handle = nullptr;
	}

}
