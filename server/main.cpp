#include "ckernel.h"
#include <QDebug>
#include <QThread>
#include <csignal>
#include <QAtomicInteger>

QAtomicInteger<bool> g_running(true);

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_running = false;
        CKernel::pKernel->endServer();
    }
}

int main() {
    qDebug() << "IM Server starting...";

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    CKernel kernel;

    if (!kernel.startServer()) {
        qDebug() << "启动服务器失败";
        return 1;
    }

    qDebug() << "Server is running on port 6789...";

    while (g_running) {
        QThread::sleep(1);
    }

    qDebug() << "Server shutting down...";
    kernel.endServer();

    return 0;
}
