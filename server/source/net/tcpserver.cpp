#include "tcpserver.h"
#include "task_handler.h"
#include "inetmediator.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <QDebug>

TcpServer* g_tcpServer = nullptr;

TcpServer::TcpServer(INetMediator* p)
    : run(true), m_epollFd(-1), m_threadPool(nullptr) {
    m_pMediator = p;
    g_tcpServer = this;
}

TcpServer::~TcpServer() {
    if (m_threadPool) {
        delete m_threadPool;
        m_threadPool = nullptr;
    }
}

bool TcpServer::initNet() {
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        qDebug() << "sock error:" << errno;
        return false;
    }
    qDebug() << "sock success";

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(_DEF_TCP_PORT));
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int opt = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(m_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        qDebug() << "bind error:" << errno;
        return false;
    }
    qDebug() << "bind success";

    if (listen(m_sock, _DEF_TCP_LISTEN_MAX) < 0) {
        qDebug() << "listen error:" << errno;
        return false;
    }
    qDebug() << "listen success";

    m_epollFd = epoll_create(10000);
    if (m_epollFd < 0) {
        qDebug() << "epoll_create error:" << errno;
        return false;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = m_sock;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_sock, &ev) < 0) {
        qDebug() << "epoll_ctl error:" << errno;
        return false;
    }

    m_threadPool = new ThreadPool(10, 100, 1000);

    qDebug() << "Server is running on port 6789...";
    return true;
}

bool TcpServer::sendData(char* data, int len, unsigned long to) {
    if (nullptr == data || len <= 0) {
        qDebug() << "data error";
        return false;
    }

    int clientFd = static_cast<int>(to);

    int totalSent = 0;
    int remaining = sizeof(int);
    char* lenBuf = reinterpret_cast<char*>(&len);

    while (remaining > 0) {
        int nSendNum = send(clientFd, lenBuf + totalSent, remaining, 0);
        if (nSendNum < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            qDebug() << "len send error:" << errno;
            return false;
        }
        totalSent += nSendNum;
        remaining -= nSendNum;
    }

    totalSent = 0;
    while (len > 0) {
        int nSendNum = send(clientFd, data + totalSent, len, 0);
        if (nSendNum < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            qDebug() << "send error:" << errno;
            return false;
        }
        totalSent += nSendNum;
        len -= nSendNum;
    }

    return true;
}

void TcpServer::recvData() {
    struct epoll_event events[10000];
    int readynum;

    while (run) {
        readynum = epoll_wait(m_epollFd, events, 10000, 1000);

        for (int i = 0; i < readynum; ++i) {
            int fd = events[i].data.fd;

            if (fd == m_sock) {
                if (events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                    if (m_threadPool) {
                        int* arg = new int(m_sock);
                        m_threadPool->addTask(taskNewConnection, arg);
                    }
                }
            } else {
                if (events[i].events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) {
                    if (m_threadPool) {
                        int* arg = new int(fd);
                        m_threadPool->addTask(taskClientData, arg);
                    }
                }
            }
        }
    }
}

void TcpServer::handleClientData(int clientFd) {
    int packLen = 0;
    int nOffset = 0;
    int nRecvNum = 0;

    qDebug() << "handleClientData called for fd:" << clientFd;

    nRecvNum = recv(clientFd, (char*)&packLen, sizeof(int), 0);
    qDebug() << "recv packLen:" << packLen << "bytes, nRecvNum=" << nRecvNum;

    if (nRecvNum > 0) {
        if (packLen <= 0 || packLen > 1024 * 1024) {
            qDebug() << "invalid packLen:" << packLen;
            return;
        }

        char* packBuf = new char[packLen];
        if (!packBuf) {
            qDebug() << "memory allocation failed";
            return;
        }
        memset(packBuf, 0, packLen);

        int remaining = packLen;
        while (remaining > 0) {
            nRecvNum = recv(clientFd, packBuf + nOffset, remaining, 0);
            if (nRecvNum > 0) {
                nOffset += nRecvNum;
                remaining -= nRecvNum;
            } else if (nRecvNum == 0) {
                qDebug() << "client closed connection:" << clientFd;
                delete[] packBuf;
                removeClient(clientFd);
                return;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                qDebug() << "recv err:" << errno;
                delete[] packBuf;
                removeClient(clientFd);
                return;
            }
        }

        qDebug() << "Data received complete! packLen=" << packLen << "nOffset=" << nOffset;
        m_pMediator->transmateData(packBuf, nOffset, clientFd);
    } else if (nRecvNum == 0) {
        qDebug() << "client disconnected:" << clientFd;
        removeClient(clientFd);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            qDebug() << "recv failed:" << errno;
            removeClient(clientFd);
        }
    }
}

void TcpServer::registerClientSocket(int clientFd) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP | EPOLLHUP;
    ev.data.fd = clientFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
        qDebug() << "epoll_ctl add client error:" << errno;
        close(clientFd);
    }
}

void TcpServer::unregisterClientSocket(int clientFd) {
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
    m_pMediator->onClientDisconnected(clientFd);
}

void TcpServer::rearmClientSocket(int clientFd) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP | EPOLLHUP;
    ev.data.fd = clientFd;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, clientFd, &ev) < 0) {
        qDebug() << "epoll_ctl mod client error:" << errno;
        removeClient(clientFd);
    }
}

void TcpServer::processClientData(char* data, int len, int clientFd) {
    m_pMediator->transmateData(data, len, clientFd);
}

void TcpServer::removeClient(int clientFd) {
    {
        QMutexLocker locker(&m_clientInfoMutex);
        m_clientInfo.remove(clientFd);
    }

    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
    m_pMediator->onClientDisconnected(clientFd);
}

void TcpServer::unInitNet() {
    run = false;

    if (m_epollFd >= 0) {
        close(m_epollFd);
        m_epollFd = -1;
    }

    if (m_sock >= 0) {
        close(m_sock);
        m_sock = -1;
    }

    if (m_threadPool) {
        delete m_threadPool;
        m_threadPool = nullptr;
    }
}
