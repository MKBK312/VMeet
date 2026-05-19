#pragma once

#include "inet.h"
#include "threadpool.h"
#include <QMap>
#include <QMutex>

#define _DEF_TCP_PORT (6789)
#define _DEF_TCP_LISTEN_MAX (100)

class TcpServer : public INet {
public:
    TcpServer(INetMediator* p);
    ~TcpServer();

    bool initNet() override;
    bool sendData(char* data, int len, unsigned long to) override;
    void recvData() override;
    void unInitNet() override;

    void handleClientData(int clientFd);
    void registerClientSocket(int clientFd);
    void unregisterClientSocket(int clientFd);
    void rearmClientSocket(int clientFd);
    void processClientData(char* data, int len, int clientFd);
    void removeClient(int clientFd);

private:

    bool run;
    int m_epollFd;

    ThreadPool* m_threadPool;
    QMap<int, struct sockaddr_in> m_clientInfo;
    QMutex m_clientInfoMutex;
};
