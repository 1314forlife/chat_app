#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QObject>
#include <QImage>
#include <QQueue>
#include <QMutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class VideoEncoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder();

    // 初始化编码器
    bool init(int width, int height, int fps = 30, int bitrate = 2000000);

    // 编码一帧
    bool encodeFrame(const QImage &frame);

    // 获取编码后的数据
signals:
    void encodedData(const QByteArray &data, bool isKeyFrame);

private:
    bool sendFrame(AVFrame *frame);
    void receivePackets();

    AVCodecContext *m_codecCtx;
    SwsContext *m_swsCtx;
    AVFrame *m_frame;
    AVPacket *m_pkt;
    int m_width;
    int m_height;
    bool m_initialized;
    int m_lastInputWidth = 0;
    int m_lastInputHeight = 0;
};

#endif // VIDEOENCODER_H