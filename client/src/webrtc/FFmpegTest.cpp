#include "FFmpegTest.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

FFmpegTest::FFmpegTest(QObject *parent)
    : QObject(parent)
{
}

void FFmpegTest::printVersion()
{
    qDebug() << "📦 FFmpeg 版本信息:";
    qDebug() << "   avcodec   :" << avcodec_version();
    qDebug() << "   avformat  :" << avformat_version();
    qDebug() << "   avutil    :" << avutil_version();
    qDebug() << "   swscale   :" << swscale_version();
    qDebug() << "   swresample:" << swresample_version();
}

bool FFmpegTest::testFFmpeg()
{
    qDebug() << "🔍 测试 FFmpeg 集成...";

    try {
        // 1. 测试查找编码器（注意 const）
        const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (codec) {
            qDebug() << "✅ H.264 编码器可用:" << codec->name;
        } else {
            qDebug() << "⚠️ H.264 编码器不可用";
        }

        // 2. 测试分配 AVFrame
        AVFrame *frame = av_frame_alloc();
        if (frame) {
            qDebug() << "✅ AVFrame 分配成功";
            av_frame_free(&frame);
        } else {
            qDebug() << "❌ AVFrame 分配失败";
            return false;
        }

        // 3. 测试分配 AVPacket
        AVPacket *pkt = av_packet_alloc();
        if (pkt) {
            qDebug() << "✅ AVPacket 分配成功";
            av_packet_free(&pkt);
        } else {
            qDebug() << "❌ AVPacket 分配失败";
            return false;
        }

        // 4. 测试分配 AVFormatContext
        AVFormatContext *fmtCtx = nullptr;
        int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, "mp4", nullptr);
        if (ret >= 0 && fmtCtx) {
            qDebug() << "✅ AVFormatContext 分配成功";
            avformat_free_context(fmtCtx);
        } else {
            qDebug() << "❌ AVFormatContext 分配失败";
            return false;
        }

        qDebug() << "✅ FFmpeg 所有测试通过！";
        emit testResult("FFmpeg 集成成功");
        return true;

    } catch (const std::exception &e) {
        qDebug() << "❌ FFmpeg 测试异常:" << e.what();
        emit testResult("FFmpeg 测试失败: " + QString(e.what()));
        return false;
    }
}