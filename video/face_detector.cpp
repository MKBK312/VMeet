#include "face_detector.h"
#include <cstring>
#include "facedetectcnn.h"

FaceDetector::FaceDetector()
    : m_pBuffer(nullptr)
{
    m_pBuffer = new unsigned char[0x20000];
}

FaceDetector::~FaceDetector()
{
    delete[] m_pBuffer;
    m_pBuffer = nullptr;
}

bool FaceDetector::init()
{
    // Buffer is already allocated in constructor.
    return true;
}

QVector<QRect> FaceDetector::detect(const QImage& rgbImage)
{
    QVector<QRect> faces;

    if (rgbImage.isNull())
        return faces;

    // scaled() with SmoothTransformation may change format away from RGB888
    QImage img = rgbImage;
    if (img.format() != QImage::Format_RGB888)
        img = img.convertToFormat(QImage::Format_RGB888);

    int width  = img.width();
    int height = img.height();
    int bytesPerPixel = 3;
    int step = width * bytesPerPixel;

    // Convert RGB to BGR (swap R and B channels)
    unsigned char* bgrData = new unsigned char[step * height];

    for (int y = 0; y < height; ++y)
    {
        const unsigned char* srcRow = img.scanLine(y);
        unsigned char* dstRow = bgrData + y * step;

        for (int x = 0; x < width; ++x)
        {
            int srcIdx = x * 3;
            int dstIdx = x * 3;

            // RGB -> BGR: swap R and B
            dstRow[dstIdx + 0] = srcRow[srcIdx + 2]; // B
            dstRow[dstIdx + 1] = srcRow[srcIdx + 1]; // G
            dstRow[dstIdx + 2] = srcRow[srcIdx + 0]; // R
        }
    }

    // Run face detection
    int* result = facedetect_cnn(m_pBuffer, bgrData, width, height, step);

    if (result)
    {
        int faceCount = result[0];
        // result[0] is int, then face records are 16 shorts each
        short* faceData = reinterpret_cast<short*>(
                           reinterpret_cast<char*>(result) + sizeof(int));
        for (int i = 0; i < faceCount; ++i)
        {
            short* p = faceData + 16 * i;
            int x = static_cast<int>(p[1]);
            int y = static_cast<int>(p[2]);
            int w = static_cast<int>(p[3]);
            int h = static_cast<int>(p[4]);

            faces.append(QRect(x, y, w, h));
        }
    }

    delete[] bgrData;
    return faces;
}
