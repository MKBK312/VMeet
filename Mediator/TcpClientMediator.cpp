#include"TcpClientMediator.h"
#include"../net/TCPClient.h"

TcpClientMediator::TcpClientMediator() {
    m_pNet = new TCPClient(this);
}
TcpClientMediator::~TcpClientMediator() {
	if (m_pNet) {
		m_pNet->unInitNet();
		delete m_pNet;
		m_pNet = nullptr;
	}
}
//打开网络
bool TcpClientMediator::openNet() {
	return m_pNet->initNet();
}
//关闭网络
void TcpClientMediator::closeNet() {
	m_pNet->unInitNet();
}
//发送数据
bool TcpClientMediator::sendData(char* data, int len, unsigned long to) {
	return m_pNet->sendData(data, len, to);
}
//转发数据（把net类接收到的数据转给核心处理类）
void TcpClientMediator::transmateData(char* data, int len, unsigned long from) {
    Q_EMIT sig_dealData(data,len,from);
}


