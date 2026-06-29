#include "VideoCallDialog.h"

VideoCallDialog::VideoCallDialog(const QString &peerName, bool isCaller, QWidget *parent)
    : QDialog(parent)
    , m_isCaller(isCaller) // ✨ 1. 保存身份状态
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    resize(720, 480);
    setStyleSheet("QDialog { background: #111827; border: 1px solid rgba(255,255,255,0.1); border-radius: 12px; }");

    // 主布局
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部状态提示
    auto *header = new QLabel(QString("📹 与 %1 视频通话中... (%2)").arg(peerName, isCaller ? "呼叫方" : "接听方"), this);
    header->setStyleSheet("color: rgba(255,255,255,0.7); font-size: 14px; padding: 12px 20px; font-weight: 500;");
    mainLayout->addWidget(header);

    // 视频主容器
    auto *videoContainer = new QWidget(this);
    videoContainer->setStyleSheet("background: #030712;");

    // 远端大画面（撑满容器）
    m_remoteVideo = new QLabel(videoContainer);
    m_remoteVideo->setAlignment(Qt::AlignCenter);
    m_remoteVideo->setText("⏳ 等待远端画面...");
    m_remoteVideo->setStyleSheet("color: rgba(255,255,255,0.3); font-size: 14px;");
    m_remoteVideo->resize(720, 400);

    // 本地小预览（右下角悬浮）
    m_localVideo = new QLabel(videoContainer);
    m_localVideo->setFixedSize(160, 120);
    m_localVideo->setStyleSheet("background: #1f2937; border: 2px solid #8b5cf6; border-radius: 6px; color: white;");
    m_localVideo->setAlignment(Qt::AlignCenter);
    m_localVideo->setText("[ 本地预览 ]");
    m_localVideo->move(720 - 160 - 16, 400 - 120 - 16); // 定位到右下角

    mainLayout->addWidget(videoContainer, 1);

    // 底部控制栏
    auto *controlBar = new QWidget(this);
    controlBar->setFixedHeight(60);
    auto *controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setAlignment(Qt::AlignCenter); // 让按钮居中摆放

    // ✨ 2. 初始化 挂断/拒绝 按钮与 新增的接听按钮
    m_hangUpBtn = new QPushButton(this);
    m_hangUpBtn->setFixedSize(120, 36);

    m_acceptBtn = new QPushButton(this);
    m_acceptBtn->setFixedSize(120, 36);

    // 根据发起方/接收方分流 UI 状态
    if (m_isCaller) {
        // 【发起方】：只有挂断按钮
        m_hangUpBtn->setText("🛑 挂断");
        m_hangUpBtn->setStyleSheet(
            "QPushButton { background: #ef4444; color: white; border: none; border-radius: 8px; font-weight: bold; }"
            "QPushButton:hover { background: #dc2626; }"
            "QPushButton:pressed { background: #b91c1c; }"
            );
        controlLayout->addWidget(m_hangUpBtn);

        m_acceptBtn->setVisible(false); // 隐藏接听按钮
    } else {
        // 【接收方】：左边接听（绿），右边拒绝（红）
        m_acceptBtn->setText("✅ 接听");
        m_acceptBtn->setStyleSheet(
            "QPushButton { background: #10b981; color: white; border: none; border-radius: 8px; font-weight: bold; }"
            "QPushButton:hover { background: #059669; }"
            "QPushButton:pressed { background: #047857; }"
            );
        m_acceptBtn->setVisible(true);
        controlLayout->addWidget(m_acceptBtn);

        // 间距占位
        controlLayout->addSpacing(20);

        m_hangUpBtn->setText("❌ 拒绝");
        m_hangUpBtn->setStyleSheet(
            "QPushButton { background: #f97316; color: white; border: none; border-radius: 8px; font-weight: bold; }"
            "QPushButton:hover { background: #ea580c; }"
            "QPushButton:pressed { background: #c2410c; }"
            );
        controlLayout->addWidget(m_hangUpBtn);
    }

    mainLayout->addWidget(controlBar);

    // ✨ 3. 信号槽绑定
    connect(m_hangUpBtn, &QPushButton::clicked, this, &VideoCallDialog::hangUpClicked);

    connect(m_acceptBtn, &QPushButton::clicked, this, [this]() {
        m_acceptBtn->setVisible(false); // 点接听后隐藏自己
        m_hangUpBtn->setText("🛑 挂断");   // 拒绝变成挂断
        m_hangUpBtn->setStyleSheet(     // 切换为标准的正红色挂断样式
            "QPushButton { background: #ef4444; color: white; border: none; border-radius: 8px; font-weight: bold; }"
            "QPushButton:hover { background: #dc2626; }"
            "QPushButton:pressed { background: #b91c1c; }"
            );
        emit acceptClicked();           // 抛出接听信号
    });
}

void VideoCallDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) { m_dragging = true; m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft(); event->accept(); }
}
void VideoCallDialog::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) { move(event->globalPosition().toPoint() - m_dragStartPos); event->accept(); }
}
void VideoCallDialog::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) { m_dragging = false; event->accept(); }
}