#include "task_handler.h"
#include "tcpserver.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern TcpServer* g_tcpServer;

void taskNewConnection(void* arg) {
    int listenFd = *(int*)arg;
    delete (int*)arg;

    // ET + listen socket: loop accept until EAGAIN
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);

        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // all pending connections accepted
            }
            std::cerr << "[Task] Accept error: " << strerror(errno) << std::endl;
            break;
        }

        char cip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, cip, INET_ADDRSTRLEN);
        std::cout << "[Task] New connection from " << cip
                  << ":" << ntohs(clientAddr.sin_port)
                  << " socket fd=" << clientFd << std::endl;

        int flag = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd, F_SETFL, flag | O_NONBLOCK);

        if (g_tcpServer) {
            g_tcpServer->registerClientSocket(clientFd);
        }
    }
}

void taskClientData(void* arg) {
    int clientFd = *(int*)arg;
    delete (int*)arg;

    std::cout << "[Task] Handling data from client fd=" << clientFd << std::endl;

    // Drain all available data on this fd.
    // EPOLLONESHOT guarantees no other thread will touch this fd.
    while (true) {
        int packLen = 0;
        int nRecvNum = recv(clientFd, (char*)&packLen, sizeof(int), 0);

        if (nRecvNum > 0) {
            if (packLen <= 0 || packLen > 1024 * 1024) {
                std::cerr << "[Task] Invalid packLen: " << packLen << std::endl;
                if (g_tcpServer) {
                    g_tcpServer->unregisterClientSocket(clientFd);
                }
                return;
            }

            char* packBuf = new char[packLen];
            memset(packBuf, 0, packLen);

            int nOffset = 0;
            int remaining = packLen;
            bool disconnected = false;
            bool error = false;

            while (remaining > 0) {
                nRecvNum = recv(clientFd, packBuf + nOffset, remaining, 0);

                if (nRecvNum > 0) {
                    nOffset += nRecvNum;
                    remaining -= nRecvNum;
                } else if (nRecvNum == 0) {
                    std::cout << "[Task] Client closed: " << clientFd << std::endl;
                    delete[] packBuf;
                    if (g_tcpServer) {
                        g_tcpServer->unregisterClientSocket(clientFd);
                    }
                    return;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;  // wait for more data
                    }
                    std::cerr << "[Task] Recv error: " << strerror(errno) << std::endl;
                    delete[] packBuf;
                    if (g_tcpServer) {
                        g_tcpServer->unregisterClientSocket(clientFd);
                    }
                    return;
                }
            }

            std::cout << "[Task] Data received complete! packLen=" << packLen << std::endl;

            if (g_tcpServer) {
                g_tcpServer->processClientData(packBuf, nOffset, clientFd);
            }

            // Loop back to try to read the next message on this fd
            continue;

        } else if (nRecvNum == 0) {
            std::cout << "[Task] Client disconnected: " << clientFd << std::endl;
            if (g_tcpServer) {
                g_tcpServer->unregisterClientSocket(clientFd);
            }
            return;

        } else {
            // nRecvNum < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // All available data drained. Re-arm for next notification.
                std::cout << "[Task] All data drained on fd=" << clientFd << ", re-arming" << std::endl;
                if (g_tcpServer) {
                    g_tcpServer->rearmClientSocket(clientFd);
                }
                return;
            } else {
                std::cerr << "[Task] Recv failed: " << strerror(errno) << std::endl;
                if (g_tcpServer) {
                    g_tcpServer->unregisterClientSocket(clientFd);
                }
                return;
            }
        }
    }
}
