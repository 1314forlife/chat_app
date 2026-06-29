#include "WebRTCManager.h"
#include "ipccapture.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

WebRTCManager::WebRTCManager(QObject *parent)
    : QObject(parent)
    , m_isSender(false)
    , m_isEncoderInitialized(false)
{
}

WebRTCManager::~WebRTCManager() {
    m_capture.stopCamera();

    if (m_ipcWorker) {
        m_ipcWorker->stop(); // 安全停止
        m_ipcWorker->quit();
        m_ipcWorker->wait();
    }

    m_remoteTrack.reset(); // ✨ 释放新轨道
    m_videoTrack.reset();
    m_peerConnection.reset();
}

void WebRTCManager::init(bool isSender) {
    m_isSender = isSender;

    // 1. 初始化标准配置
    rtc::Configuration config;
    m_peerConnection = std::make_shared<rtc::PeerConnection>(config);

    // 2. 配置基础网络回调
    setupPeerConnection();

    if (m_isSender) {
        // ============================================================
        // 【发送端链路】：本地摄像头 -> 编码器 -> WebRTC 发送
        //                ✨同时支持：接收被叫方传回的 IPC 画面并解码
        // ============================================================

        // ✨ 确保主叫方也初始化了解码器，防止收到对方流时空指针崩溃
        m_decoder.init();
        connect(&m_decoder, &VideoDecoder::frameReady, this, &WebRTCManager::remoteFrameReady);

        // ✨ 主叫方监听被叫方的 IPC 视频轨
        m_peerConnection->onTrack([this](std::shared_ptr<rtc::Track> track) {
            qDebug() << "📹 [调试] onTrack 触发! 轨道 ID:" << track->mid().c_str()
                    << "方向:" << (int)track->direction(); // 检查方向是否正确

            m_remoteTrack = track;

            m_remoteTrack->onMessage([this](rtc::message_variant data) {
                if (std::holds_alternative<rtc::binary>(data)) {
                    auto& binaryData = std::get<rtc::binary>(data);
                    // 埋点：查看收到的包大小
                    qDebug() << "📦 [调试] 收到远端二进制包，大小:" << binaryData.size() << "字节";

                    QByteArray h264Frame(reinterpret_cast<const char*>(binaryData.data()),
                                         static_cast<int>(binaryData.size()));

                    QMetaObject::invokeMethod(this, [this, h264Frame]() {
                        // 埋点：查看解码器是否真的被触发
                        qDebug() << "🎬 [调试] 正在向解码器发送数据，大小:" << h264Frame.size();
                        m_decoder.decodeFrame(h264Frame);
                    }, Qt::QueuedConnection);
                }
            });
        });

        connect(&m_capture, &VideoCapture::frameReady, this, [this](const QImage &frame) {
            if (frame.isNull()) return;
            emit localFrameReady(frame);

            if (!m_isEncoderInitialized) {
                m_encoder.init(frame.width(), frame.height(), 30, 2000000);
                m_isEncoderInitialized = true;
            }
            m_encoder.encodeFrame(frame);
        });

        connect(&m_encoder, &VideoEncoder::encodedData, this, [this](const QByteArray &data, bool isKeyFrame) {
            if (m_videoTrack && m_videoTrack->isOpen()) {
                const auto *rtcData = reinterpret_cast<const std::byte*>(data.constData());

                // 埋点：确认发送的数据包是否合规
                qDebug() << "📤 [调试] 正在向轨道发送数据包，大小:" << data.size()
                         << "关键帧:" << isKeyFrame;

                m_videoTrack->send(rtcData, static_cast<size_t>(data.size()));
            } else {
                qDebug() << "❌ [调试] 发送失败！videoTrack 未打开或为空";
            }
        });

        rtc::Description::Video trackDesc("video", rtc::Description::Direction::SendRecv); // 改为双向
        trackDesc.addH264Codec(96);
        m_videoTrack = m_peerConnection->addTrack(trackDesc);

        // ✨ 重点：为轨道添加事件监听，监控它什么时候真的打开了
        m_videoTrack->onOpen([this]() {
            qDebug() << "🚀 [核心] 视频轨道已成功打开，可以发送数据！";
        });

        m_videoTrack->onClosed([this]() {
            qDebug() << "🛑 [核心] 视频轨道已关闭！";
        });

        m_capture.startCamera();
        createOffer();
    }
    else {
        // ============================================================
        // 【接收端链路】：IPC 摄像头 -> 编码器 -> 发送给主叫
        //                同时接收主叫过来的画面 -> 解码器 -> UI
        // ============================================================

        m_decoder.init();
        connect(&m_decoder, &VideoDecoder::frameReady, this, &WebRTCManager::remoteFrameReady);

        // 监听远端流
        m_peerConnection->onTrack([this](std::shared_ptr<rtc::Track> track) {
            qDebug() << "📹 被叫方成功捕捉到远端媒体轨道！";
            m_remoteTrack = track; // 🔒 核心修复：赋值给远程轨道，保护本地 m_videoTrack 不被覆盖

            m_remoteTrack->onMessage([this](rtc::message_variant data) {
                if (std::holds_alternative<rtc::binary>(data)) {
                    auto& binaryData = std::get<rtc::binary>(data);
                    QByteArray h264Frame(reinterpret_cast<const char*>(binaryData.data()),
                                         static_cast<int>(binaryData.size()));

                    QMetaObject::invokeMethod(this, [this, h264Frame]() {
                        m_decoder.decodeFrame(h264Frame);
                    }, Qt::QueuedConnection);
                }
            });
        });

        // 绑定本地 IPC 编码输出 -> 通过本地发送轨推送出去
        connect(&m_encoder, &VideoEncoder::encodedData, this, [this](const QByteArray &data, bool isKeyFrame) {
            Q_UNUSED(isKeyFrame);
            if (m_videoTrack && m_videoTrack->isOpen()) {
                const auto *rtcData = reinterpret_cast<const std::byte*>(data.constData());
                m_videoTrack->send(rtcData, static_cast<size_t>(data.size()));
            }
        });

        // 创建本地的发送轨
        rtc::Description::Video trackDesc("video", rtc::Description::Direction::SendRecv);
        trackDesc.addH264Codec(96);
        m_videoTrack = m_peerConnection->addTrack(trackDesc);

        // 启动本地 IPC 异步拉流预览
        QString rtspUrl = "rtsp://admin:z13312555281@192.168.0.101:554/stream1";
        qDebug() << "🚀 被叫方正在启动 IPC 拉流线程，目标地址:" << rtspUrl;

        if (m_ipcWorker) {
            m_ipcWorker->stop();
            m_ipcWorker->quit();
            m_ipcWorker->wait();
            m_ipcWorker->deleteLater();
        }

        m_ipcWorker = new IpcCaptureWorker(rtspUrl, this);

        connect(m_ipcWorker, &IpcCaptureWorker::frameCaptured, this, [this](const QImage &frame) {
            if (frame.isNull()) return;

            // 🌟 核心时序：只要一收到帧，立刻抛出 localFrameReady 信号
            QImage smallFrame = frame.scaled(1280, 720, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            emit localFrameReady(smallFrame);

            // 只有当本地发送轨开启成功后，才开始喂数据给编码器，节省性能并防止崩溃
            if (m_videoTrack && m_videoTrack->isOpen()) {
                if (!m_isEncoderInitialized) {
                    m_encoder.init(frame.width(), frame.height(), 30, 2000000);
                    m_isEncoderInitialized = true;
                }
                m_encoder.encodeFrame(frame);
            }
        });

        m_ipcWorker->start();
    }
}

void WebRTCManager::setupPeerConnection() {
    m_peerConnection->onLocalCandidate([this](rtc::Candidate candidate) {
        QJsonObject json;
        json["candidate"] = QString::fromStdString(candidate.candidate());
        json["mid"] = QString::fromStdString(candidate.mid());

        QJsonDocument doc(json);
        emit sendSignaling("candidate", doc.toJson(QJsonDocument::Compact));
    });
    m_peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        qDebug() << "🌐 [调试] WebRTC 链路状态变更至:" << (int)state;

        // 关键点：当状态为 Connected 时，强制检查轨道是否已就绪
        if (state == rtc::PeerConnection::State::Connected) {
            qDebug() << "✅ WebRTC 已连接，正在检查视频轨道...";
            if (m_videoTrack) {
                // 如果 libdatachannel 版本支持，在这里尝试一次手动开启
                qDebug() << "🔍 轨道状态: " << (m_videoTrack->isOpen() ? "已开启" : "未开启");
            }
        }
    });
    m_peerConnection->onStateChange([](rtc::PeerConnection::State state) {
        qDebug() << "🌐 WebRTC 链路状态变更:" << static_cast<int>(state);
    });
}

void WebRTCManager::createOffer() {
    if (!m_peerConnection) return;
    m_peerConnection->setLocalDescription();
    auto sdp = m_peerConnection->localDescription();
    if (sdp) {
        emit sendSignaling("offer", QString::fromStdString(std::string(sdp.value())));
    }
}

void WebRTCManager::receiveSignaling(const QString &type, const QString &sdp)
{
    // 将 sdp 参数转为 rtc::Description 对象（或者你原本的格式）
    auto description = rtc::Description(sdp.toStdString());

    if (type == "offer") {
        qDebug() << "📥 [WebRTCManager] 收到远端 Offer，准备设置...";

        // 【关键】设置远端 Offer
        m_peerConnection->setRemoteDescription(description);

        // 【关键】创建并设置 Answer
        m_peerConnection->setLocalDescription();
        auto answer = m_peerConnection->localDescription();
        if (answer) {
            QString sdpStr = QString::fromStdString(std::string(answer.value()));

            // ✨ 强制修改 Answer 的角色为 passive，确保符合协议
            if (sdpStr.contains("a=setup:actpass")) {
                sdpStr.replace("a=setup:actpass", "a=setup:passive");
            }
            emit sendSignaling("answer", QString::fromStdString(std::string(answer.value())));
        }

        m_hasRemoteDescription = true;
        // ... (后面保持你现在的缓存倾倒逻辑不变)
        for (const QString &cachedCandidate : m_pendingRemoteCandidates) {
            this->receiveSignaling("candidate", cachedCandidate);
        }
        m_pendingRemoteCandidates.clear();
    }
    else if (type == "answer") {
        qDebug() << "📥 [WebRTCManager] 收到远端 Answer，正在预处理...";

        // 💡 核心修复：清理 SDP 中的非法角色
        QString cleanSdp = sdp;
        // 强制将 actpass 改为 active，避免底层解析校验失败
        if (cleanSdp.contains("a=setup:actpass")) {
            cleanSdp.replace("a=setup:actpass", "a=setup:active");
            qDebug() << "🛠 [WebRTCManager] 已修复 SDP 角色: actpass -> active";
        }

        // 将清理后的 SDP 转换为 description
        auto description = rtc::Description(cleanSdp.toStdString());

        // 现在再喂进去，就不会报 Illegal role 错误了
        m_peerConnection->setRemoteDescription(description);

        m_hasRemoteDescription = true;

        // 释放缓存
        for (const QString &cachedCandidate : m_pendingRemoteCandidates) {
            this->receiveSignaling("candidate", cachedCandidate);
        }
        m_pendingRemoteCandidates.clear();
    }
    else if (type == "candidate") {
        if (!m_hasRemoteDescription) {
            m_pendingRemoteCandidates.append(sdp);
            return;
        }

        // 【关键】解析并喂入 Candidate
        // 假设 sdp 参数里存的是 JSON 格式的 candidate，你需要解析它
        QJsonDocument doc = QJsonDocument::fromJson(sdp.toUtf8());
        QJsonObject obj = doc.object();
        std::string cand = obj["candidate"].toString().toStdString();
        std::string mid = obj["mid"].toString().toStdString();

        // 调用底层接口喂入
        m_peerConnection->addRemoteCandidate(rtc::Candidate(cand, mid));

        qDebug() << "🚀 [WebRTCManager] Candidate 已成功安全喂入底层";
    }
}