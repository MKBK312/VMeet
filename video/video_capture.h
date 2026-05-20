#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <QObject>
#include <QByteArray>
#include <QCamera>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QImage>

class FaceDetector;

class VideoCapture : public QObject
{
    Q_OBJECT
public:
    explicit VideoCapture(QObject *parent = nullptr);
    ~VideoCapture();

    void start();
    void stop();

signals:
    void sig_videoFrameReady(QByteArray jpegData);  // JPEG encoded frame
    void sig_rawFrameReady(QImage rgbImage);        // raw RGB frame for H.264 encoding

private slots:
    void slot_frameProbed(const QVideoFrame &frame);

private:
    QCamera*      m_pCamera;
    QVideoProbe*  m_pProbe;
    FaceDetector* m_pFaceDetector;
};

#endif // VIDEO_CAPTURE_H
