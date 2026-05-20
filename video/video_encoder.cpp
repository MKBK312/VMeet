#include "video_encoder.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

VideoEncoder::VideoEncoder(QObject *parent)
    : QObject(parent), m_pCtx(nullptr), m_pFrame(nullptr), m_pPacket(nullptr),
      m_pSwsCtx(nullptr), m_width(0), m_height(0), m_initialized(false), m_pts(0)
{
}

VideoEncoder::~VideoEncoder()
{
    cleanup();
}

bool VideoEncoder::init(int width, int height)
{
    if (m_initialized && width == m_width && height == m_height) return true;
    cleanup();

    m_width = width;
    m_height = height;

    AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        qDebug() << "[VideoEncoder] libx264 not found, trying mpeg4 fallback";
        codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    }
    if (!codec) {
        qDebug() << "[VideoEncoder] No usable video encoder found";
        return false;
    }
    qDebug() << "[VideoEncoder] using codec:" << codec->name;

    m_pCtx = avcodec_alloc_context3(codec);
    if (!m_pCtx) return false;

    m_pCtx->width = width;
    m_pCtx->height = height;
    m_pCtx->time_base = AVRational{1, 15};
    m_pCtx->framerate = AVRational{15, 1};
    m_pCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pCtx->max_b_frames = 0;
    m_pCtx->gop_size = 15;

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(m_pCtx->priv_data, "preset", "ultrafast", 0);
        av_opt_set(m_pCtx->priv_data, "crf", "30", 0);
    }

    if (avcodec_open2(m_pCtx, codec, nullptr) < 0) {
        qDebug() << "[VideoEncoder] avcodec_open2 failed";
        cleanup();
        return false;
    }

    m_pFrame = av_frame_alloc();
    m_pFrame->format = m_pCtx->pix_fmt;
    m_pFrame->width = width;
    m_pFrame->height = height;
    if (av_frame_get_buffer(m_pFrame, 0) < 0) {
        qDebug() << "[VideoEncoder] av_frame_get_buffer failed";
        cleanup();
        return false;
    }

    m_pPacket = av_packet_alloc();

    m_pSwsCtx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                               width, height, AV_PIX_FMT_YUV420P,
                               SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_pSwsCtx) {
        qDebug() << "[VideoEncoder] sws_getContext failed";
        cleanup();
        return false;
    }

    m_initialized = true;
    m_pts = 0;
    qDebug() << "[VideoEncoder] initialized" << width << "x" << height;
    return true;
}

void VideoEncoder::cleanup()
{
    if (m_pSwsCtx) { sws_freeContext(m_pSwsCtx); m_pSwsCtx = nullptr; }
    if (m_pPacket) { av_packet_free(&m_pPacket); m_pPacket = nullptr; }
    if (m_pFrame) { av_frame_free(&m_pFrame); m_pFrame = nullptr; }
    if (m_pCtx) { avcodec_free_context(&m_pCtx); m_pCtx = nullptr; }
    m_initialized = false;
}

void VideoEncoder::encode(const QImage& rgbImage)
{
    if (rgbImage.isNull()) return;
    if (!init(rgbImage.width(), rgbImage.height())) return;

    // Convert RGB24 → YUV420P
    const uint8_t* srcSlice[1] = { rgbImage.constBits() };
    int srcStride[1] = { static_cast<int>(rgbImage.bytesPerLine()) };
    sws_scale(m_pSwsCtx, srcSlice, srcStride, 0, m_height,
              m_pFrame->data, m_pFrame->linesize);

    m_pFrame->pts = m_pts++;

    int ret = avcodec_send_frame(m_pCtx, m_pFrame);
    if (ret < 0) {
        qDebug() << "[VideoEncoder] avcodec_send_frame error:" << ret;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(m_pCtx, m_pPacket);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) {
            qDebug() << "[VideoEncoder] avcodec_receive_packet error:" << ret;
            break;
        }
        QByteArray ba(reinterpret_cast<const char*>(m_pPacket->data), m_pPacket->size);
        emit sig_encodedFrame(ba);
        av_packet_unref(m_pPacket);
    }
}
