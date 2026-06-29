#include "VideoDecoder.h"
#include <QDebug>
#include <libavutil/imgutils.h>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent), m_codecCtx(nullptr), m_swsCtx(nullptr)
    , m_frame(nullptr), m_pkt(nullptr), m_initialized(false)
{
}

VideoDecoder::~VideoDecoder()
{
    if (m_codecCtx) avcodec_free_context(&m_codecCtx);
    if (m_swsCtx) sws_freeContext(m_swsCtx);
    if (m_frame) av_frame_free(&m_frame);
    if (m_pkt) av_packet_free(&m_pkt);
}

bool VideoDecoder::init()
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) return false;

    m_codecCtx = avcodec_alloc_context3(codec);
    // 关键优化：开启多线程解码，减少单帧耗时
    m_codecCtx->thread_count = 0;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) return false;

    m_frame = av_frame_alloc();
    m_pkt = av_packet_alloc();
    m_initialized = true;
    return true;
}

bool VideoDecoder::decodeFrame(const QByteArray &data)
{
    if (!m_initialized) return false;

    // 使用 av_packet_ref 或者直接赋值并 unref
    m_pkt->data = (uint8_t*)data.data();
    m_pkt->size = data.size();

    int ret = avcodec_send_packet(m_codecCtx, m_pkt);
    // 必须清空 packet 引用，防止内存泄漏或二次引用
    av_packet_unref(m_pkt);

    if (ret < 0) return false;
    return receiveFrame();
}

bool VideoDecoder::receiveFrame()
{
    while (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
        int width = m_frame->width;
        int height = m_frame->height;

        // 初始化或重建转换器
        if (!m_swsCtx || m_cachedWidth != width || m_cachedHeight != height) {
            if (m_swsCtx) sws_freeContext(m_swsCtx);
            m_swsCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                                      width, height, AV_PIX_FMT_RGB24,
                                      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
            m_cachedWidth = width;
            m_cachedHeight = height;
        }

        // 优化：内存复用
        int rgbSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
        if (m_rgbBuffer.size() < rgbSize) {
            m_rgbBuffer.resize(rgbSize);
        }

        uint8_t *rgbPlanes[1] = {(uint8_t*)m_rgbBuffer.data()};
        int rgbLinesize[1] = {width * 3};

        sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, height, rgbPlanes, rgbLinesize);

        // 使用 copy() 创建 QImage 的深拷贝，因为 rgbBuffer 是复用的
        QImage image((uchar*)m_rgbBuffer.data(), width, height, QImage::Format_RGB888);
        emit frameReady(image.copy());

        av_frame_unref(m_frame);
    }
    return true;
}