#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <QImage>
#include <QVector>
#include <QRect>

class FaceDetector
{
public:
    FaceDetector();
    ~FaceDetector();

    bool init();
    QVector<QRect> detect(const QImage& rgbImage);

private:
    unsigned char* m_pBuffer;
};

#endif // FACE_DETECTOR_H
