#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

#include <QObject>
#include <QByteArray>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

class AudioEncoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioEncoder(QObject *parent = nullptr);
    ~AudioEncoder();

    void encode(const QByteArray& pcmData);

signals:
    void sig_encodedFrame(QByteArray opusData);

private:
    bool init();
    void cleanup();

    AVCodecContext* m_pCtx;
    AVFrame*        m_pFrame;
    AVPacket*       m_pPacket;
    bool            m_initialized;
    int64_t         m_pts;
};

#endif // AUDIO_ENCODER_H
