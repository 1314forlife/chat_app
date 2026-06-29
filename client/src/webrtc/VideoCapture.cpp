#include <QtGlobal>
#include "VideoCapture.h"
#include <QDebug>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QVideoSink>       // 确保引入了接收器头文件
#include <QVideoFrame>      // 确保引入了帧头文件

VideoCapture::VideoCapture(QObject *parent)
    : QObject(parent)
    , m_camera(nullptr)
    , m_captureSession(nullptr)
    , m_videoSink(nullptr)
    , m_isRunning(false)
{
}

VideoCapture::~VideoCapture()
{
    stopCamera();
}

bool VideoCapture::startCamera(const QString &deviceName)
{
    if (m_isRunning) {
        stopCamera();
    }

    QCameraDevice cameraDevice;
    if (deviceName.isEmpty()) {
        auto cameras = QMediaDevices::videoInputs();
        if (cameras.isEmpty()) {
            qDebug() << "❌ 没有找到摄像头";
            return false;
        }
        cameraDevice = cameras.first();
    } else {
        for (const auto &cam : QMediaDevices::videoInputs()) {
            if (cam.description() == deviceName) {
                cameraDevice = cam;
                break;
            }
        }
    }

    if (cameraDevice.isNull()) {
        qDebug() << "❌ 无法找到摄像头";
        return false;
    }

    // 创建摄像头
    m_camera = new QCamera(cameraDevice, this);

    // 创建视频接收器
    m_videoSink = new QVideoSink(this);

    // 用旧版字符串语法强接信号，完美绕过编译期的成员检查！
    // 运行时 Qt 会自动在你的 QVideoSink 里查找它支持的那一个信号。
    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &VideoCapture::onVideoFrame);

    // 创建捕获会话
    m_captureSession = new QMediaCaptureSession(this);
    m_captureSession->setCamera(m_camera);
    m_captureSession->setVideoSink(m_videoSink);

    // 启动
    m_camera->start();
    m_isRunning = true;
    qDebug() << "✅ 摄像头已启动:" << cameraDevice.description();
    return true;
}

void VideoCapture::stopCamera()
{
    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_captureSession) {
        delete m_captureSession;
        m_captureSession = nullptr;
    }
    if (m_videoSink) {
        delete m_videoSink;
        m_videoSink = nullptr;
    }
    m_isRunning = false;
    qDebug() << "⏹️ 摄像头已停止";
}

bool VideoCapture::isRunning() const
{
    return m_isRunning;
}

void VideoCapture::onVideoFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        return;
    }

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) {
        return;
    }

    QImage image;

    QImage::Format format = QVideoFrameFormat::imageFormatFromPixelFormat(cloneFrame.pixelFormat());
    if (format != QImage::Format_Invalid) {
        image = QImage(cloneFrame.bits(0),
                       cloneFrame.width(),
                       cloneFrame.height(),
                       cloneFrame.bytesPerLine(0),
                       format);
    } else {
        QImage converted = cloneFrame.toImage();
        if (!converted.isNull()) {
            image = converted;
        }
    }

    cloneFrame.unmap();

    if (!image.isNull()) {
        emit frameReady(image);
    }
}

#include "VideoCapture.moc"