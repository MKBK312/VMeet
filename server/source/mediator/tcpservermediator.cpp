#include "inetmediator.h"
#include "tcpserver.h"
#include "tcpservermediator.h"
#include "ckernel.h"
#include <QDebug>
#include <QThread>

TcpServerMediator::TcpServerMediator() : m_recvThread(nullptr) {
    m_pNet = new TcpServer(this);
}

TcpServerMediator::~TcpServerMediator() {
    if (m_recvThread) {
        m_recvThread->quit();
        m_recvThread->wait();
        delete m_recvThread;
        m_recvThread = nullptr;
    }
    if (m_pNet) {
        m_pNet->unInitNet();
        delete m_pNet;
        m_pNet = nullptr;
    }
}

bool TcpServerMediator::openNet() {
    return m_pNet->initNet();
}

void TcpServerMediator::closeNet() {
    m_pNet->unInitNet();
}

bool TcpServerMediator::sendData(char* data, int len, unsigned long to) {
    return m_pNet->sendData(data, len, to);
}

void TcpServerMediator::transmateData(char* data, int len, unsigned long from) {
    CKernel::pKernel->dealData(data, len, from);
}

void TcpServerMediator::startRecv() {
    m_recvThread = QThread::create([this]() {
        m_pNet->recvData();
    });
    m_recvThread->start();
}

void TcpServerMediator::closeClientSocket(unsigned long socket) {
    if (m_pNet) {
        ((TcpServer*)m_pNet)->removeClient((int)socket);
    }
}

void TcpServerMediator::onClientDisconnected(unsigned long socket) {
    if (CKernel::pKernel) {
        CKernel::pKernel->removeUserBySocket(socket);
    }
}
