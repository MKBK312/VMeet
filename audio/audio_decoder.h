#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <QObject>
#include <QByteArray>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

class AudioDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder();

    void decode(const QByteArray& opusData);

signals:
    void sig_decodedFrame(QByteArray pcmData);

private:
    bool init();
    void cleanup();

    AVCodecContext* m_pCtx;
    AVFrame*        m_pFrame;
    AVPacket*       m_pPacket;
    bool            m_initialized;
};

#endif // AUDIO_DECODER_H
