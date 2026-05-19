#include "screen_capture.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QImage>
#include <QBuffer>
#include <QDebug>

ScreenCapture::ScreenCapture(QObject *parent)
    : QObject(parent), m_pTimer(nullptr), m_running(false)
{
}

ScreenCapture::~ScreenCapture()
{
    stop();
}

void ScreenCapture::start()
{
    qDebug() << __func__;
    if (m_running) return;
    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, &ScreenCapture::slot_captureFrame);
    m_pTimer->start(100);  // ~10fps
    m_running = true;
    qDebug() << __func__ << "screen capture started";
}

void ScreenCapture::stop()
{
    qDebug() << __func__;
    m_running = false;
    if (m_pTimer) {
        m_pTimer->stop();
        delete m_pTimer;
        m_pTimer = nullptr;
    }
}

void ScreenCapture::slot_captureFrame()
{
    if (!m_running) return;
    QScreen *screen = QApplication::primaryScreen();
    if (!screen) return;

    QPixmap map = screen->grabWindow(QApplication::desktop()->winId());
    QImage image = map.toImage();
    image = image.scaled(960, 540, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray jpeg;
    QBuffer buf(&jpeg);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "JPEG", 50);

    qDebug() << __func__ << "screen frame JPEG:" << jpeg.size() << "bytes";
    emit sig_screenFrameReady(jpeg);
}
