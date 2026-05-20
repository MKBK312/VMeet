#include "audio_decoder.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

AudioDecoder::AudioDecoder(QObject *parent)
    : QObject(parent), m_pCtx(nullptr), m_pFrame(nullptr), m_pPacket(nullptr),
      m_initialized(false)
{
}

AudioDecoder::~AudioDecoder()
{
    cleanup();
}

bool AudioDecoder::init()
{
    if (m_initialized) return true;

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    if (!codec) {
        qDebug() << "[AudioDecoder] Opus decoder not found";
        return false;
    }
    qDebug() << "[AudioDecoder] using decoder:" << codec->name;

    m_pCtx = avcodec_alloc_context3(codec);
    if (!m_pCtx) return false;

    if (avcodec_open2(m_pCtx, codec, nullptr) < 0) {
        qDebug() << "[AudioDecoder] avcodec_open2 failed";
        cleanup();
        return false;
    }

    m_pFrame = av_frame_alloc();
    m_pPacket = av_packet_alloc();
    m_initialized = true;
    qDebug() << "[AudioDecoder] initialized";
    return true;
}

void AudioDecoder::cleanup()
{
    if (m_pPacket) { av_packet_free(&m_pPacket); m_pPacket = nullptr; }
    if (m_pFrame) { av_frame_free(&m_pFrame); m_pFrame = nullptr; }
    if (m_pCtx) { avcodec_free_context(&m_pCtx); m_pCtx = nullptr; }
    m_initialized = false;
}

void AudioDecoder::decode(const QByteArray& opusData)
{
    if (opusData.isEmpty()) return;
    if (!init()) return;

    m_pPacket->data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(opusData.constData()));
    m_pPacket->size = opusData.size();

    int ret = avcodec_send_packet(m_pCtx, m_pPacket);
    if (ret < 0) {
        qDebug() << "[AudioDecoder] avcodec_send_packet error:" << ret;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_pCtx, m_pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "[AudioDecoder] avcodec_receive_frame error:" << ret;
            break;
        }

        // Output raw PCM: 16-bit mono, nb_samples * 2 bytes
        int pcmSize = m_pFrame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)m_pFrame->format);
        if (av_get_channel_layout_nb_channels(m_pFrame->channel_layout) > 1) {
            pcmSize *= av_get_channel_layout_nb_channels(m_pFrame->channel_layout);
        }
        QByteArray pcm(reinterpret_cast<const char*>(m_pFrame->data[0]), pcmSize);
        emit sig_decodedFrame(pcm);
    }
}
