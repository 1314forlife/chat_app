#ifndef FFMPEGTEST_H
#define FFMPEGTEST_H

#include <QObject>
#include <QString>

class FFmpegTest : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegTest(QObject *parent = nullptr);

    // 测试 FFmpeg 是否可用
    bool testFFmpeg();

    // 打印版本信息
    void printVersion();

signals:
    void testResult(const QString &message);
};

#endif // FFMPEGTEST_H