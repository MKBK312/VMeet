#pragma once

#include "inetmediator.h"
#include <QThread>

class TcpServerMediator : public INetMediator {
public:
    TcpServerMediator();
    ~TcpServerMediator();

    bool openNet() override;
    void closeNet() override;
    bool sendData(char* data, int len, unsigned long to) override;
    void transmateData(char* data, int len, unsigned long from) override;
    void startRecv() override;
    void onClientDisconnected(unsigned long socket) override;
    void closeClientSocket(unsigned long socket) override;

private:
    QThread* m_recvThread;
};
