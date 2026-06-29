#include "WebRTCTest.h"
#include <rtc/rtc.hpp>
#include <QDebug>

WebRTCTest::WebRTCTest(QObject *parent)
    : QObject(parent)
{
}

void WebRTCTest::printVersion()
{
    qDebug() << "📦 libdatachannel 测试";
    qDebug() << "✅ 头文件加载成功";

    try {
        rtc::PeerConnection pc;
        qDebug() << "✅ PeerConnection 创建成功";
    } catch (const std::exception &e) {
        qDebug() << "❌ PeerConnection 创建失败:" << e.what();
    }
}

bool WebRTCTest::testBasicConnection()
{
    qDebug() << "🔍 开始 WebRTC 基本测试...";

    try {
        // 1. 创建配置
        rtc::Configuration config;

        // ✨【修复1】使用带有完整 STUN URL 的构造函数初始化 IceServer
        rtc::IceServer server("stun:stun.l.google.com:19302");
        config.iceServers.push_back(server);

        // 2. 创建 PeerConnection
        auto pc = std::make_shared<rtc::PeerConnection>(config);

        // 3. 创建 DataChannel
        auto dc = pc->createDataChannel("test");

        // 4. 设置回调
        dc->onOpen([]() {
            qDebug() << "✅ DataChannel 已打开";
        });

        dc->onMessage([](std::variant<rtc::binary, std::string> message) {
            if (std::holds_alternative<std::string>(message)) {
                auto msg = std::get<std::string>(message);
                qDebug() << "📩 收到消息:" << QString::fromStdString(msg);
            }
        });

        // 5. 创建 Offer
        pc->onLocalDescription([](const rtc::Description &desc) {
            qDebug() << "✅ SDP Offer 创建成功";
            qDebug() << "📋 SDP 类型:" << desc.typeString().c_str();

            // ✨【修复2】将 Description 显式转为 std::string 来获取 SDP 文本及长度
            std::string sdpStr = std::string(desc);
            qDebug() << "📋 SDP 长度:" << sdpStr.size();
        });

        pc->onLocalCandidate([](const rtc::Candidate &candidate) {
            // ✨【修复3】candidate() 是个方法，必须加括号调用
            std::string candStr = candidate.candidate();
            qDebug() << "🧊 ICE 候选:" << QString::fromStdString(candStr);
        });

        // 创建 Offer
        pc->createOffer();

        qDebug() << "✅ WebRTC 测试完成";
        emit testResult("WebRTC 测试通过");

        return true;

    } catch (const std::exception &e) {
        qDebug() << "❌ WebRTC 测试失败:" << e.what();
        emit testResult("WebRTC 测试失败: " + QString(e.what()));
        return false;
    }
}