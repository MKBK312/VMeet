#pragma once

class INetMediator;

class INet {
public:
    INet() : m_sock(-1), m_pMediator(nullptr) {}
    virtual ~INet() {}
    virtual bool initNet() = 0;
    virtual bool sendData(char* data, int len, unsigned long to) = 0;
    virtual void recvData() = 0;
    virtual void unInitNet() = 0;
    virtual void handleClientData(int clientFd) {}

protected:
    int m_sock;
    INetMediator* m_pMediator;
};