#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>
#include <QQueue>
#include <QMutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    bool init();
    bool decodeFrame(const QByteArray &data);

signals:
    void frameReady(const QImage &frame);

private:
    bool receiveFrame();

    AVCodecContext *m_codecCtx;
    SwsContext *m_swsCtx;
    AVFrame *m_frame;
    AVPacket *m_pkt;
    int m_width;
    int m_height;
    int m_cachedWidth;   // ✅ 缓存尺寸
    int m_cachedHeight;  // ✅ 缓存尺寸
    bool m_initialized;
    QByteArray m_rgbBuffer;
};

#endif // VIDEODECODER_H