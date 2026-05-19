#include "video_capture.h"
#include <QAbstractVideoBuffer>
#include <QBuffer>
#include <QCameraInfo>
#include <QDebug>

VideoCapture::VideoCapture(QObject *parent) : QObject(parent), m_pCamera(nullptr), m_pProbe(nullptr)
{
    qDebug() << __func__ << "available cameras:" << QCameraInfo::availableCameras().size();
}

VideoCapture::~VideoCapture()
{
    stop();
}

void VideoCapture::start()
{
    qDebug() << __func__;
    if (m_pCamera) {
        qDebug() << __func__ << "camera already running";
        return;
    }
    QList<QCameraInfo> cams = QCameraInfo::availableCameras();
    if (cams.isEmpty()) {
        qDebug() << __func__ << "NO CAMERA FOUND";
        return;
    }
    qDebug() << __func__ << "using camera:" << cams.first().description();
    m_pCamera = new QCamera(cams.first(), this);
    m_pProbe = new QVideoProbe(this);
    if (m_pProbe->setSource(m_pCamera)) {
        connect(m_pProbe, &QVideoProbe::videoFrameProbed,
                this, &VideoCapture::slot_frameProbed);
        m_pCamera->start();
        qDebug() << __func__ << "camera started OK";
    } else {
        qDebug() << __func__ << "video probe FAILED - camera not usable";
        delete m_pProbe; m_pProbe = nullptr;
        delete m_pCamera; m_pCamera = nullptr;
    }
}

void VideoCapture::stop()
{
    qDebug() << __func__;
    if (m_pCamera) {
        m_pCamera->stop();
        delete m_pCamera;
        m_pCamera = nullptr;
        qDebug() << __func__ << "camera stopped";
    }
    m_pProbe = nullptr;
}

void VideoCapture::slot_frameProbed(const QVideoFrame &frame)
{
    QVideoFrame f(frame);
    if (!f.map(QAbstractVideoBuffer::ReadOnly)) return;

    QImage rgb;
    QImage::Format fmt = QVideoFrame::imageFormatFromPixelFormat(f.pixelFormat());

    if (fmt != QImage::Format_Invalid) {
        // Direct mapping (RGB32, ARGB32, etc.)
        QImage img(f.bits(), f.width(), f.height(), f.bytesPerLine(), fmt);
        rgb = img.convertToFormat(QImage::Format_RGB888);
    } else if (f.pixelFormat() == QVideoFrame::Format_YUYV) {
        // Manual YUYV → RGB (BT.601)
        rgb = QImage(f.width(), f.height(), QImage::Format_RGB888);
        const uchar* src = f.bits();
        int srcStride = f.bytesPerLine();
        for (int y = 0; y < f.height(); y++) {
            uchar* dst = rgb.scanLine(y);
            for (int x = 0; x < f.width(); x += 2) {
                int idx = x * 2;
                int y0 = src[idx];
                int u  = src[idx + 1] - 128;
                int y1 = src[idx + 2];
                int v  = src[idx + 3] - 128;

                // BT.601 YUV → RGB (integer arithmetic)
                int yv0 = (y0 - 16) * 298;
                int yv1 = (y1 - 16) * 298;

                int dx = x * 3;
                // R = 1.164*(Y-16) + 1.596*(V-128)
                dst[dx + 2] = qBound(0, (yv0 + 409 * v + 128) >> 8, 255);
                // G = 1.164*(Y-16) - 0.813*(V-128) - 0.391*(U-128)
                dst[dx + 1] = qBound(0, (yv0 - 100 * u - 208 * v + 128) >> 8, 255);
                // B = 1.164*(Y-16) + 2.018*(U-128)
                dst[dx]     = qBound(0, (yv0 + 516 * u + 128) >> 8, 255);
                // Pixel 2
                dst[dx + 5] = qBound(0, (yv1 + 409 * v + 128) >> 8, 255);
                dst[dx + 4] = qBound(0, (yv1 - 100 * u - 208 * v + 128) >> 8, 255);
                dst[dx + 3] = qBound(0, (yv1 + 516 * u + 128) >> 8, 255);
            }
            src += srcStride;
        }
    } else if (f.pixelFormat() == QVideoFrame::Format_UYVY) {
        rgb = QImage(f.width(), f.height(), QImage::Format_RGB888);
        const uchar* src = f.bits();
        int srcStride = f.bytesPerLine();
        for (int y = 0; y < f.height(); y++) {
            uchar* dst = rgb.scanLine(y);
            for (int x = 0; x < f.width(); x += 2) {
                int idx = x * 2;
                int u  = src[idx] - 128;
                int y0 = src[idx + 1];
                int v  = src[idx + 2] - 128;
                int y1 = src[idx + 3];

                int yv0 = (y0 - 16) * 298;
                int yv1 = (y1 - 16) * 298;

                int dx = x * 3;
                dst[dx + 2] = qBound(0, (yv0 + 409 * v + 128) >> 8, 255);
                dst[dx + 1] = qBound(0, (yv0 - 100 * u - 208 * v + 128) >> 8, 255);
                dst[dx]     = qBound(0, (yv0 + 516 * u + 128) >> 8, 255);
                dst[dx + 5] = qBound(0, (yv1 + 409 * v + 128) >> 8, 255);
                dst[dx + 4] = qBound(0, (yv1 - 100 * u - 208 * v + 128) >> 8, 255);
                dst[dx + 3] = qBound(0, (yv1 + 516 * u + 128) >> 8, 255);
            }
            src += srcStride;
        }
    } else {
        qDebug() << __func__ << "unsupported format:" << f.pixelFormat();
        f.unmap();
        return;
    }
    f.unmap();

    rgb = rgb.scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray jpeg;
    QBuffer buf(&jpeg);
    buf.open(QIODevice::WriteOnly);
    rgb.save(&buf, "JPEG", 70);

    emit sig_videoFrameReady(jpeg);
}
