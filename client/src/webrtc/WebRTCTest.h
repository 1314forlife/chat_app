#ifndef WEBRTCTEST_H
#define WEBRTCTEST_H

#include <QObject>
#include <QString>

class WebRTCTest : public QObject
{
    Q_OBJECT

public:
    explicit WebRTCTest(QObject *parent = nullptr);

    // 测试 libdatachannel 是否正常工作
    bool testBasicConnection();

    // 打印版本信息
    void printVersion();

signals:
    void testResult(const QString &message);
};

#endif // WEBRTCTEST_H