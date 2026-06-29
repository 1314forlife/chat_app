#ifndef IPCCAPTURE_H
#define IPCCAPTURE_H
#include <QThread>
#include <QImage>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <atomic>

// 专门用来拉取 IPC 摄像头 RTSP 流的子线程
class IpcCaptureWorker : public QThread {
    Q_OBJECT
public:
    IpcCaptureWorker(const QString &url, QObject *parent = nullptr)
        : QThread(parent), m_url(url), m_running(true) {}

    void stop() { m_running = false; }

signals:
    // 每当 OpenCV 抓到一帧画面，就通过这个信号发给 WebRTCManager
    void frameCaptured(const QImage &image);

protected:
    void run() override {
        // ✨ 核心修复 1：在构造或 open 时显式强制指定 cv::CAP_FFMPEG 后端
        cv::VideoCapture cap;
        if (!cap.open(m_url.toStdString(), cv::CAP_FFMPEG)) {
            qDebug() << "❌ OpenCV 无法打开 IPC 摄像头 (即使尝试了 FFMPEG 后端):" << m_url;
            return;
        } else {
            qDebug() << "✅ OpenCV 成功通过 FFMPEG 后端打开 IPC 摄像头！";
        }

        cv::Mat frame;
        cv::Mat rgbFrame; // ✨ 核心优化：专门准备一个变量处理颜色转换，防止污染原 buffer

        while (m_running) {
            if (!cap.read(frame) || frame.empty()) {
                QThread::msleep(10);
                continue;
            }

            // 将 OpenCV 的 BGR 格式转换为 Qt 的 RGB 格式
            cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

            // 使用转换后的 rgbFrame 构造 QImage
            QImage img((const unsigned char*)(rgbFrame.data), rgbFrame.cols, rgbFrame.rows,
                       rgbFrame.step, QImage::Format_RGB888);

            emit frameCaptured(img.copy()); // 发射信号（img.copy() 会深拷贝数据，线程安全）

            QThread::msleep(30); // 1000ms / 30fps ≈ 33ms，微调为 30ms 配合拉流节奏
        }
        cap.release();
    }

private:
    QString m_url;
    std::atomic<bool> m_running;
};
#endif // IPCCAPTURE_H