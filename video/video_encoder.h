#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <QObject>
#include <QImage>
#include <QByteArray>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class VideoEncoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder();

    void encode(const QImage& rgbImage);

signals:
    void sig_encodedFrame(QByteArray h264Data);

private:
    bool init(int width, int height);
    void cleanup();

    AVCodecContext* m_pCtx;
    AVFrame*        m_pFrame;
    AVPacket*       m_pPacket;
    SwsContext*     m_pSwsCtx;
    int             m_width;
    int             m_height;
    bool            m_initialized;
    int64_t         m_pts;
};

#endif // VIDEO_ENCODER_H
