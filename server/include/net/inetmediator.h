        #pragma once

#include "inet.h"

class INetMediator {
public:
    INetMediator() : m_pNet(nullptr) {}
    virtual ~INetMediator() {}
    virtual bool openNet() = 0;
    virtual void closeNet() = 0;
    virtual bool sendData(char* data, int len, unsigned long to) = 0;
    virtual void transmateData(char* data, int len, unsigned long from) = 0;
    virtual void startRecv() {}
    virtual void onClientDisconnected(unsigned long socket) {}
    virtual void closeClientSocket(unsigned long socket) {}

protected:
    INet* m_pNet;
};
