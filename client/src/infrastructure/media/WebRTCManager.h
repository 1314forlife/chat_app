#pragma once

#include <QObject>
#include <QByteArray>
#include <QImage>
#include <memory>
#include <string>
#include <vector>
#include "ipccapture.h"

// libdatachannel 核心
#include "rtc/rtc.hpp"

// 已有的多媒体业务组件
#include "webrtc/VideoCapture.h"
#include "webrtc/VideoEncoder.h"
#include "webrtc/VideoDecoder.h"

class WebRTCManager : public QObject {
    Q_OBJECT
public:
    explicit WebRTCManager(QObject *parent = nullptr);
    ~WebRTCManager();

    // 初始化：指定是发送端还是接收端
    void init(bool isSender);

    // 接收来自 WebSocket 的信令并喂给 WebRTC
    void receiveSignaling(const QString &type, const QString &sdp);

signals:
    // 产生本地信令，通知 MainWindow 转发出去
    void sendSignaling(const QString &type, const QString &sdp);

    // UI 渲染信号
    void localFrameReady(const QImage &frame);
    void remoteFrameReady(const QImage &frame);

private:
    void setupPeerConnection();
    void createOffer();

    bool m_isSender = false;
    bool m_isEncoderInitialized = false;


    std::shared_ptr<rtc::PeerConnection> m_peerConnection;
    std::shared_ptr<rtc::Track>          m_videoTrack;


    VideoCapture m_capture;
    VideoEncoder m_encoder;
    VideoDecoder m_decoder;

    IpcCaptureWorker *m_ipcWorker = nullptr;
    std::shared_ptr<rtc::Track> m_remoteTrack = nullptr;

    bool m_hasRemoteDescription = false;       // 标记是否已经成功设置了远端 SDP
    QList<QString> m_pendingRemoteCandidates;  // 用于在 SDP 到达前缓存先跑过来的 Candidate
};