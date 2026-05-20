#include "video_decoder.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent), m_pCtx(nullptr), m_pFrame(nullptr), m_pPacket(nullptr),
      m_pSwsCtx(nullptr), m_initialized(false), m_width(0), m_height(0)
{
}

VideoDecoder::~VideoDecoder()
{
    cleanup();
}

bool VideoDecoder::init()
{
    if (m_initialized) return true;

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qDebug() << "[VideoDecoder] H.264 decoder not found, trying mpeg4";
        codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
    }
    if (!codec) {
        qDebug() << "[VideoDecoder] No usable video decoder found";
        return false;
    }
    qDebug() << "[VideoDecoder] using decoder:" << codec->name;

    m_pCtx = avcodec_alloc_context3(codec);
    if (!m_pCtx) return false;

    if (avcodec_open2(m_pCtx, codec, nullptr) < 0) {
        qDebug() << "[VideoDecoder] avcodec_open2 failed";
        cleanup();
        return false;
    }

    m_pFrame = av_frame_alloc();
    m_pPacket = av_packet_alloc();
    m_initialized = true;
    qDebug() << "[VideoDecoder] initialized";
    return true;
}

void VideoDecoder::cleanup()
{
    if (m_pSwsCtx) { sws_freeContext(m_pSwsCtx); m_pSwsCtx = nullptr; }
    if (m_pPacket) { av_packet_free(&m_pPacket); m_pPacket = nullptr; }
    if (m_pFrame) { av_frame_free(&m_pFrame); m_pFrame = nullptr; }
    if (m_pCtx) { avcodec_free_context(&m_pCtx); m_pCtx = nullptr; }
    m_initialized = false;
    m_width = m_height = 0;
}

void VideoDecoder::decode(const QByteArray& h264Data, int userId)
{
    if (h264Data.isEmpty()) return;
    if (!init()) return;

    m_pPacket->data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(h264Data.constData()));
    m_pPacket->size = h264Data.size();

    int ret = avcodec_send_packet(m_pCtx, m_pPacket);
    if (ret < 0) {
        qDebug() << "[VideoDecoder] avcodec_send_packet error:" << ret;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_pCtx, m_pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "[VideoDecoder] avcodec_receive_frame error:" << ret;
            break;
        }

        int w = m_pFrame->width;
        int h = m_pFrame->height;
        if (w != m_width || h != m_height) {
            if (m_pSwsCtx) { sws_freeContext(m_pSwsCtx); m_pSwsCtx = nullptr; }
            m_pSwsCtx = sws_getContext(w, h, (AVPixelFormat)m_pFrame->format,
                                       w, h, AV_PIX_FMT_RGB24,
                                       SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
            m_width = w;
            m_height = h;
        }

        if (!m_pSwsCtx) return;

        QImage rgb(w, h, QImage::Format_RGB888);
        uint8_t* dstSlice[1] = { rgb.bits() };
        int dstStride[1] = { static_cast<int>(rgb.bytesPerLine()) };
        sws_scale(m_pSwsCtx, m_pFrame->data, m_pFrame->linesize, 0, h,
                  dstSlice, dstStride);

        emit sig_decodedFrame(rgb, userId);
    }
}
