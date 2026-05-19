#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QScreen>

class ScreenCapture : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCapture(QObject *parent = nullptr);
    ~ScreenCapture();

    void start();
    void stop();

signals:
    void sig_screenFrameReady(QByteArray jpegData);

private slots:
    void slot_captureFrame();

private:
    QTimer*  m_pTimer;
    bool     m_running;
};

#endif // SCREEN_CAPTURE_H
