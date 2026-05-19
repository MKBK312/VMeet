#include"UdpNetMediator.h"
#include"../net/UDP.h"

UdpNetMediator::UdpNetMediator() {
	m_pNet = new Udp(this);
}
UdpNetMediator::~UdpNetMediator() {
	if (m_pNet) {
		closeNet();
		delete m_pNet;
		m_pNet = nullptr;
	}
}
//打开网络
bool UdpNetMediator::openNet() {
	return m_pNet->initNet();
}
//关闭网络
void UdpNetMediator::closeNet() {
	m_pNet->unInitNet();
}
//发送数据
bool UdpNetMediator::sendData(char* data, int len, unsigned long to) {
	return m_pNet->sendData(data, len, to);
}
//转发数据（把net类接收到的数据转给核心处理类）
void UdpNetMediator::transmateData(char* data, int len, unsigned long from) {
	//传给核心处理类
	
	//测试代码：打印接收到的数据
	cout << "UdpNetMediator packBuf" << data << endl;
}