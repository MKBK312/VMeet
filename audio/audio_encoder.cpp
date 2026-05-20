#include "audio_encoder.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

AudioEncoder::AudioEncoder(QObject *parent)
    : QObject(parent), m_pCtx(nullptr), m_pFrame(nullptr), m_pPacket(nullptr),
      m_initialized(false), m_pts(0)
{
}

AudioEncoder::~AudioEncoder()
{
    cleanup();
}

bool AudioEncoder::init()
{
    if (m_initialized) return true;

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    if (!codec) {
        qDebug() << "[AudioEncoder] Opus encoder not found";
        return false;
    }
    qDebug() << "[AudioEncoder] using codec:" << codec->name;

    m_pCtx = avcodec_alloc_context3(codec);
    if (!m_pCtx) return false;

    m_pCtx->sample_rate = 8000;
    m_pCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    m_pCtx->channel_layout = AV_CH_LAYOUT_MONO;
    m_pCtx->channels = av_get_channel_layout_nb_channels(m_pCtx->channel_layout);
    m_pCtx->time_base = AVRational{1, 8000};

    if (avcodec_open2(m_pCtx, codec, nullptr) < 0) {
        qDebug() << "[AudioEncoder] avcodec_open2 failed";
        cleanup();
        return false;
    }

    m_pFrame = av_frame_alloc();
    m_pFrame->format = m_pCtx->sample_fmt;
    m_pFrame->channel_layout = m_pCtx->channel_layout;
    m_pFrame->sample_rate = m_pCtx->sample_rate;
    m_pFrame->nb_samples = 320;  // 320 samples = 20ms @ 8kHz (16-bit mono = 640 bytes)
    if (av_frame_get_buffer(m_pFrame, 0) < 0) {
        qDebug() << "[AudioEncoder] av_frame_get_buffer failed";
        cleanup();
        return false;
    }

    m_pPacket = av_packet_alloc();
    m_initialized = true;
    m_pts = 0;
    qDebug() << "[AudioEncoder] initialized";
    return true;
}

void AudioEncoder::cleanup()
{
    if (m_pPacket) { av_packet_free(&m_pPacket); m_pPacket = nullptr; }
    if (m_pFrame) { av_frame_free(&m_pFrame); m_pFrame = nullptr; }
    if (m_pCtx) { avcodec_free_context(&m_pCtx); m_pCtx = nullptr; }
    m_initialized = false;
}

void AudioEncoder::encode(const QByteArray& pcmData)
{
    if (pcmData.size() < 640) return;
    if (!init()) return;

    // 640 bytes PCM = 320 samples (16-bit mono)
    memcpy(m_pFrame->data[0], pcmData.constData(), 640);
    m_pFrame->pts = m_pts++;

    int ret = avcodec_send_frame(m_pCtx, m_pFrame);
    if (ret < 0) {
        qDebug() << "[AudioEncoder] avcodec_send_frame error:" << ret;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_pCtx, m_pPacket);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "[AudioEncoder] avcodec_receive_packet error:" << ret;
            break;
        }
        QByteArray ba(reinterpret_cast<const char*>(m_pPacket->data), m_pPacket->size);
        emit sig_encodedFrame(ba);
        av_packet_unref(m_pPacket);
    }
}
