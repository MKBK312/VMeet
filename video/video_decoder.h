#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <QObject>
#include <QImage>
#include <QByteArray>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void decode(const QByteArray& h264Data, int userId);

signals:
    void sig_decodedFrame(QImage rgbImage, int userId);

private:
    bool init();
    void cleanup();

    AVCodecContext* m_pCtx;
    AVFrame*        m_pFrame;
    AVPacket*       m_pPacket;
    SwsContext*     m_pSwsCtx;
    bool            m_initialized;
    int             m_width;
    int             m_height;
};

#endif // VIDEO_DECODER_H
