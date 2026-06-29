#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QObject>
#include <QImage>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>

class VideoCapture : public QObject
{
    Q_OBJECT

public:
    explicit VideoCapture(QObject *parent = nullptr);
    ~VideoCapture();

    bool startCamera(const QString &deviceName = QString());
    void stopCamera();
    bool isRunning() const;

signals:
    void frameReady(const QImage &frame);

private slots:
    void onVideoFrame(const QVideoFrame &frame);

private:
    QCamera *m_camera;
    QMediaCaptureSession *m_captureSession;
    QVideoSink *m_videoSink;
    bool m_isRunning;
};

#endif // VIDEOCAPTURE_H