#include <QApplication>
#include <QDebug>
#include <memory>
#include <functional>
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "application/controllers/LoginController.h"
#include "domain/usecases/LoginUseCase.h"
#include "infrastructure/network/WebSocketClient.h"
#include "infrastructure/repositories/MemoryUserRepository.h"
#include "infrastructure/repositories/MemoryMessageRepository.h"
#include "infrastructure/serialization/JsonSerializer.h"
#include "webrtc/WebRTCTest.h"
#include "webrtc/FFmpegTest.h"
#include "webrtc/VideoCapture.h"

#include "webrtc/VideoEncoder.h"
#include "webrtc/VideoDecoder.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "🚀 启动聊天客户端...";

    // 1. 创建窗口与布局
    // QWidget window;
    // window.setWindowTitle("视频闭环测试: 采集 → 编码 → 解码 → 显示");
    // window.resize(800, 600);

    // QVBoxLayout *layout = new QVBoxLayout(&window);
    // QLabel *originalLabel = new QLabel("原始画面");
    // QLabel *decodedLabel = new QLabel("编码后再解码的画面");
    // originalLabel->setFixedSize(320, 240);
    // decodedLabel->setFixedSize(320, 240);
    // originalLabel->setStyleSheet("border: 1px solid red;");
    // decodedLabel->setStyleSheet("border: 1px solid green;");

    // QHBoxLayout *hLayout = new QHBoxLayout();
    // hLayout->addWidget(originalLabel);
    // hLayout->addWidget(decodedLabel);
    // layout->addLayout(hLayout);
    // window.show();

    // // 2. 创建核心多媒体组件
    // VideoCapture capture;
    // VideoEncoder encoder;
    // VideoDecoder decoder;

    // // 3. 初始化解码器 (解码器支持动态追随尺寸，可以优先初始化)
    // if (!decoder.init()) {
    //     qDebug() << "❌ 解码器初始化失败";
    //     return -1;
    // }

    // // 4. 🎯 【核心改动】摄像头信号路由：第一帧到来时动态初始化编码器
    // // 使用状态标志来确保编码器只需启动时配置一次
    // static bool isEncoderInitialized = false;

    // QObject::connect(&capture, &VideoCapture::frameReady,
    //                  [&originalLabel, &encoder, &isEncoderInitialized](const QImage &frame) {
    //                      if (frame.isNull()) return;

    //                      // 显示左侧原始画面 (等比例缩放)
    //                      QImage scaled = frame.scaled(320, 240, Qt::KeepAspectRatio);
    //                      originalLabel->setPixmap(QPixmap::fromImage(scaled));

    //                      // ✨ 如果编码器还没初始化，利用当前帧的硬件实际规格进行“量身定制”
    //                      if (!isEncoderInitialized) {
    //                          int realWidth = frame.width();
    //                          int realHeight = frame.height();
    //                          qDebug() << "📸 捕获成功！检测到摄像头物理规格:" << realWidth << "x" << realHeight;

    //                          // 锁死物理宽高，从此让 H.264 画布与摄像头比例 1:1 绝对重合
    //                          if (!encoder.init(realWidth, realHeight, 30, 2000000)) {
    //                              qDebug() << "❌ 编码器动态初始化失败！";
    //                              return;
    //                          }
    //                          isEncoderInitialized = true;
    //                      }

    //                      // 送去安全编码 (内部 sws_scale 现在已经具备动态 Stride 适配能力)
    //                      encoder.encodeFrame(frame);
    //                  });

    // // 5. 编码器数据流 → 喂给解码器
    // QObject::connect(&encoder, &VideoEncoder::encodedData,
    //                  [&decoder](const QByteArray &data, bool isKeyFrame) {
    //                      // 可以在稳定后注释掉此行日志以防刷屏
    //                      qDebug() << "📦 传输 H264 编码数据:" << data.size() << (isKeyFrame ? "I帧" : "P帧");
    //                      decoder.decodeFrame(data);
    //                  });

    // // 6. 解码器恢复像素 → 显示右侧画面
    // QObject::connect(&decoder, &VideoDecoder::frameReady,
    //                  [&decodedLabel](const QImage &frame) {
    //                      // 此时 frame 已经拥有完全一致的原始高宽比
    //                      QImage scaled = frame.scaled(320, 240, Qt::KeepAspectRatio);
    //                      decodedLabel->setPixmap(QPixmap::fromImage(scaled));
    //                      qDebug() << "✅ 解码器无错位还原显示";
    //                  });

    // // 7. 最终激活摄像头硬件开始采集
    // capture.startCamera();

    // WebRTCTest test;
    // test.printVersion();
    // test.testBasicConnection();

    // FFmpegTest ffTest;
    // ffTest.printVersion();
    // ffTest.testFFmpeg();

    auto userRepo = std::make_shared<MemoryUserRepository>();
    auto messageRepo = std::make_shared<MemoryMessageRepository>();
    auto wsClient = std::make_shared<WebSocketClient>();
    auto serializer = std::make_shared<JsonSerializer>();

    auto loginUseCase = std::make_shared<LoginUseCase>(userRepo, wsClient);
    auto loginController = std::make_shared<LoginController>(loginUseCase);

    auto mainWindow = std::make_shared<MainWindow>(loginUseCase, wsClient);

    LoginDialog loginDialog(loginController);

    QObject::connect(loginController.get(), &LoginController::loginSuccess,
                     std::function<void(const QVariantMap&)>([&](const QVariantMap &userData) {
                         QString username = userData["username"].toString();
                         qDebug() << "✅ 登录成功，设置当前用户:" << username;
                         mainWindow->setCurrentUser(username);
                         mainWindow->show();
                         loginDialog.accept();
                     })
                     );

    QObject::connect(loginController.get(), &LoginController::loginFailed,
                     std::function<void(const QString&)>([&](const QString &error) {
                         qDebug() << "❌ 登录失败:" << error;
                         loginDialog.show();
                     })
                     );

    loginDialog.exec();

    return app.exec();
}