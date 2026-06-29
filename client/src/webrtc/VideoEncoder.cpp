#include "VideoEncoder.h"
#include <QDebug>

VideoEncoder::VideoEncoder(QObject *parent)
    : QObject(parent)
    , m_codecCtx(nullptr)
    , m_swsCtx(nullptr)
    , m_frame(nullptr)
    , m_pkt(nullptr)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
{
}

VideoEncoder::~VideoEncoder()
{
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_pkt) {
        av_packet_free(&m_pkt);
    }
}

bool VideoEncoder::init(int width, int height, int fps, int bitrate)
{
    m_width = width;
    m_height = height;

    // 1. 查找 H.264 编码器
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        qDebug() << "❌ 找不到 H.264 编码器";
        return false;
    }

    // 2. 分配编码器上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qDebug() << "❌ 分配编码器上下文失败";
        return false;
    }

    // 3. 设置编码参数
    m_codecCtx->width = width;
    m_codecCtx->height = height;
    m_codecCtx->time_base = {1, fps};
    m_codecCtx->framerate = {fps, 1};
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_codecCtx->bit_rate = bitrate;
    m_codecCtx->gop_size = 30;
    m_codecCtx->max_b_frames = 0;

    // 4. 设置编码器选项（x264 参数）
    av_opt_set(m_codecCtx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_codecCtx->priv_data, "tune", "zerolatency", 0);

    // 5. 打开编码器
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qDebug() << "❌ 打开编码器失败";
        return false;
    }

    // 6. 分配帧和包
    m_frame = av_frame_alloc();
    m_frame->format = m_codecCtx->pix_fmt;
    m_frame->width = width;
    m_frame->height = height;
    av_frame_get_buffer(m_frame, 0);

    m_pkt = av_packet_alloc();

    // 7. 分配图像转换上下文
    m_swsCtx = sws_getContext(
        width, height, AV_PIX_FMT_RGB24,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        );

    if (!m_swsCtx) {
        qDebug() << "❌ 分配图像转换上下文失败";
        return false;
    }

    m_initialized = true;
    qDebug() << "✅ 视频编码器初始化成功" << width << "x" << height;
    return true;
}

bool VideoEncoder::encodeFrame(const QImage &frame)
{
    if (!m_initialized) return false;

    QImage rgb = frame.convertToFormat(QImage::Format_RGB888);
    if (rgb.isNull()) return false;

    // 1. 使用成员变量代替 static，保证多实例安全
    if (!m_swsCtx || m_lastInputWidth != rgb.width() || m_lastInputHeight != rgb.height()) {
        if (m_swsCtx) sws_freeContext(m_swsCtx);
        m_swsCtx = sws_getContext(rgb.width(), rgb.height(), AV_PIX_FMT_RGB24,
                                  m_width, m_height, AV_PIX_FMT_YUV420P,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        m_lastInputWidth = rgb.width();
        m_lastInputHeight = rgb.height();
    }

    const uint8_t *data[1] = {rgb.bits()};
    int linesize[1] = {static_cast<int>(rgb.bytesPerLine())};

    sws_scale(m_swsCtx, data, linesize, 0, rgb.height(), m_frame->data, m_frame->linesize);

    m_frame->pts++; // 简单的 pts 递增

    return sendFrame(m_frame);
}

bool VideoEncoder::sendFrame(AVFrame *frame)
{
    int ret = avcodec_send_frame(m_codecCtx, frame);
    if (ret < 0) {
        return false;
    }

    receivePackets();
    return true;
}

void VideoEncoder::receivePackets()
{
    while (true) {
        int ret = avcodec_receive_packet(m_codecCtx, m_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) break;

        bool isKeyFrame = (m_pkt->flags & AV_PKT_FLAG_KEY);
        // 使用拷贝，确保上层信号传递安全
        QByteArray data((const char*)m_pkt->data, m_pkt->size);
        emit encodedData(data, isKeyFrame);

        // 【关键修复】释放 Packet 内部数据，否则必定内存溢出
        av_packet_unref(m_pkt);
    }
}