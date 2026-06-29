#include "ui/MainWindow.h"
#include "infrastructure/network/WebSocketClient.h"
#include <QDebug>
#include <QDateTime>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QCloseEvent>
#include <QFile>
#include <QApplication>
#include <QScrollBar>

MainWindow::MainWindow(std::shared_ptr<LoginUseCase> loginUseCase,
                       std::shared_ptr<WebSocketClient> wsClient,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_loginUseCase(loginUseCase)
    , m_wsClient(wsClient)
{
    setupUI();

    // ============================================================
    // 1. 断线重连信号处理
    // ============================================================
    connect(m_wsClient.get(), &WebSocketClient::reconnecting, this, [this]() {
        m_statusLabel->setText("🔄 连接断开，正在重连...");
        m_statusLabel->setStyleSheet("color: #fbbf24;");
        qDebug() << "🔄 断线重连中...";
    });

    connect(m_wsClient.get(), &WebSocketClient::reconnectSuccess, this, [this]() {
        m_statusLabel->setText("✅ 重连成功！");
        m_statusLabel->setStyleSheet("color: #34d399;");
        qDebug() << "✅ 重连成功！";
        onConnectToServer(); // 重新登录
    });

    connect(m_wsClient.get(), &WebSocketClient::reconnectFailed, this, [this]() {
        m_statusLabel->setText("❌ 重连失败，请手动重启客户端");
        m_statusLabel->setStyleSheet("color: #ef4444;");
    });

    connect(m_wsClient.get(), &WebSocketClient::connected, this, &MainWindow::onConnectToServer);
    connect(m_wsClient.get(), &WebSocketClient::userListReceived, this, &MainWindow::onUserListReceived);

    // ============================================================
    // 2. 核心重构：音视频分流信号处理（解决来电时序与本地预览）
    // ============================================================

    // 【被叫方】收到远端呼叫邀请 (Offer)
    connect(m_wsClient.get(), &WebSocketClient::incomingVideoCall,
            this, [this](const QString &fromUser, const QString &sdp) {
                qDebug() << "🔔 [MainWindow] 收到视频来电，来自:" << fromUser;

                if (m_callDialog || m_rtcManager) {
                    qDebug() << "⚠️ 已经在通话中或正有呼入，忽略新呼叫";
                    return;
                }

                // 1. 弹出接听/挂断界面 (false 代表是被呼叫方)
                m_callDialog = new VideoCallDialog(fromUser, false, this);
                m_callDialog->show();

                // 2. 初始化 WebRTC 管理器
                m_rtcManager = new WebRTCManager(this);

                // 3. 绑定本地与远端画面渲染回调
                connect(m_rtcManager, &WebRTCManager::localFrameReady, this, [this](const QImage &frame) {
                    if(m_callDialog && m_callDialog->localVideoLabel())
                        m_callDialog->localVideoLabel()->setPixmap(QPixmap::fromImage(frame.scaled(160, 120, Qt::KeepAspectRatio)));
                });
                connect(m_rtcManager, &WebRTCManager::remoteFrameReady, this, [this](const QImage &frame) {
                    if(m_callDialog && m_callDialog->remoteVideoLabel())
                        m_callDialog->remoteVideoLabel()->setPixmap(QPixmap::fromImage(frame.scaled(720, 400, Qt::KeepAspectRatio)));
                });

                // 4. 应答(Answer)或网络候选点(Candidate)发给对方
                connect(m_rtcManager, &WebRTCManager::sendSignaling, this, [this, fromUser](const QString &t, const QString &s) {
                    QJsonObject json;
                    json["cmd"] = "video_rtc_signaling";
                    QJsonObject d;
                    d["to"] = fromUser;
                    d["type"] = t;
                    d["sdp"] = s;
                    json["data"] = d;
                    m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
                });

                // 5. 绑定本地点击挂断/拒绝
                connect(m_callDialog, &VideoCallDialog::hangUpClicked, this, [this, fromUser]() {
                    qDebug() << "🤙 本地点击挂断/拒绝...";

                    QJsonObject json;
                    json["cmd"] = "video_rtc_signaling";
                    QJsonObject d;
                    d["to"] = fromUser;
                    d["type"] = "candidate";
                    d["sdp"] = "hangup";
                    json["data"] = d;
                    m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));

                    if(m_callDialog) { m_callDialog->close(); m_callDialog->deleteLater(); m_callDialog = nullptr; }
                    if(m_rtcManager) { m_rtcManager->deleteLater(); m_rtcManager = nullptr; }
                });

                // ✨ 【核心时序修复 1】：不等点击接听，收到来电时立刻进行硬件与拉流初始化！
                // 这样用户在看到“接听/拒绝”弹窗的瞬间，右下角本地预览的 IPC 摄像头画面就已经活过来了。
                m_rtcManager->init(false);

                // 6. 绑定点击“接听”按钮：此时只做信令握手，开启传输通道
                connect(m_callDialog, &VideoCallDialog::acceptClicked, this, [this, sdp]() {
                    qDebug() << "🚀 用户点击接听，通过应答(Answer)激活双向音视频...";
                    if (m_rtcManager) {
                        m_rtcManager->receiveSignaling("offer", sdp); // 喂入 Offer，内部自动触发 sendSignaling 发送 Answer
                    }
                });
            });

    // 【主叫方】收到对方同意后回传的 Answer 信令
    connect(m_wsClient.get(), &WebSocketClient::incomingVideoAnswer,
            this, [this](const QString &fromUser, const QString &sdp) {
                qDebug() << "🤝 对方已接听电话！收到 Answer 信令";
                if (m_rtcManager) {
                    m_rtcManager->receiveSignaling("answer", sdp);
                }
            });

    // 【通用】收到网络打洞候选点 (Candidate)
    connect(m_wsClient.get(), &WebSocketClient::incomingIceCandidate,
            this, [this](const QString &fromUser, const QString &sdp) {
                if (sdp == "hangup") {
                    qDebug() << "远端已挂断通话";
                    if(m_callDialog) { m_callDialog->close(); m_callDialog->deleteLater(); m_callDialog = nullptr; }
                    if(m_rtcManager) { m_rtcManager->deleteLater(); m_rtcManager = nullptr; }
                    return;
                }

                if (m_rtcManager) {
                    m_rtcManager->receiveSignaling("candidate", sdp);
                }
            });


    // ============================================================
    // 3. 文本普通聊天消息与业务指令逻辑
    // ============================================================
    connect(m_wsClient.get(), &WebSocketClient::messageReceived,
            this, [this](const QString &msg) {
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
                if (!doc.isObject()) return;

                QJsonObject obj = doc.object();
                QString action = obj.value("action").toString();
                QString cmd = obj["cmd"].toString();
                QJsonObject data = obj["data"].toObject();

                if (obj.contains("from") && obj["from"].toString() == "system") {
                    QString content = obj["content"].toString();
                    QJsonDocument contentDoc = QJsonDocument::fromJson(content.toUtf8());
                    if (contentDoc.isObject()) {
                        QJsonObject contentObj = contentDoc.object();
                        if (contentObj.contains("event")) {
                            QString event = contentObj["event"].toString();
                            if (event == "user_online") {
                                QJsonObject user = contentObj["user"].toObject();
                                QString username = user["username"].toString();
                                if (username != m_currentUser) {
                                    bool exists = false;
                                    for (int i = 0; i < m_contactList->count(); i++) {
                                        if (m_contactList->item(i)->text() == username) { exists = true; break; }
                                    }
                                    if (!exists) {
                                        m_contactList->addItem(username);
                                        m_statusLabel->setText(QString("✅ %1 上线了").arg(username));
                                    }
                                }
                                return;
                            } else if (event == "user_offline") {
                                QString username = contentObj["username"].toString();
                                for (int i = 0; i < m_contactList->count(); i++) {
                                    if (m_contactList->item(i)->text() == username) {
                                        delete m_contactList->takeItem(i);
                                        m_statusLabel->setText(QString("❌ %1 下线了").arg(username));
                                        break;
                                    }
                                }
                                return;
                            }
                        }
                    }
                    return;
                }

                if(obj.contains("status") && obj["status"].toString() == "success") {
                    if (obj.contains("data") && obj["data"].isArray()) {
                        QJsonArray messages = obj["data"].toArray();
                        bool isHistory = false;
                        if (!messages.isEmpty() && messages[0].isObject()) {
                            QJsonObject first = messages[0].toObject();
                            if (first.contains("from") && first.contains("content")) isHistory = true;
                        }

                        if (isHistory) {
                            QLayoutItem *layoutItem;
                            while ((layoutItem = m_chatLayout->takeAt(0)) != nullptr) {
                                delete layoutItem->widget();
                                delete layoutItem;
                            }
                            if (messages.isEmpty()) {
                                appendMessage("系统", "暂无历史消息", false);
                            } else {
                                for (const QJsonValue &val : messages) {
                                    QJsonObject msgObj = val.toObject();
                                    QString from = msgObj["from"].toString();
                                    QString content = msgObj["content"].toString();
                                    if (from == m_currentUser) appendMessage("我", content, true);
                                    else appendMessage(from, content, false);
                                }
                            }
                            return;
                        }
                    }
                }

                if (obj.contains("from") && obj.contains("content")) {
                    QString from = obj["from"].toString();
                    QString content = obj["content"].toString();

                    if (from != "system") {
                        if (from != m_currentContact) markUnread(from);
                        if (from != m_currentUser) playNotificationSound();

                        QString key = from + content;
                        qint64 now = QDateTime::currentMSecsSinceEpoch();
                        if (key != m_lastMessageKey || now - m_lastMessageTime > 500) {
                            if (from == m_currentContact) {
                                appendMessage(from, content, false);
                            } else {
                                m_statusLabel->setText(QString("📩 来自 %1: %2").arg(from, content));
                            }
                            m_lastMessageKey = key;
                            m_lastMessageTime = now;
                        }
                    }
                    return;
                }

                if (obj.contains("status") && !obj.contains("from")) { return; }
                qDebug() << "📩 忽略非聊天消息";
            });

    connect(m_contactList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        if (item) onContactSelected(m_contactList->row(item));
    });
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowTitle("Rust Chat");
    resize(950, 620);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *titleBar = new QWidget(this);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet("QWidget#titleBar { border-bottom: 1px solid rgba(255,255,255,0.05); }");

    auto *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(16, 0, 12, 0);
    titleBarLayout->setSpacing(6);

    auto *titleText = new QLabel("✦ Rust Chat", this);
    titleText->setObjectName("titleText");
    titleText->setStyleSheet("font-size: 13px; font-weight: 500; letter-spacing: 1px; color: rgba(255,255,255,0.85);");

    auto *minBtn = new QPushButton("─", this);
    auto *maxBtn = new QPushButton("☐", this);
    auto *closeBtn = new QPushButton("✕", this);
    minBtn->setObjectName("minBtn");
    maxBtn->setObjectName("maxBtn");
    closeBtn->setObjectName("closeBtn");

    minBtn->setFixedSize(28, 22);
    maxBtn->setFixedSize(28, 22);
    closeBtn->setFixedSize(28, 22);

    titleBarLayout->addWidget(titleText);
    titleBarLayout->addStretch();
    titleBarLayout->addWidget(minBtn);
    titleBarLayout->addWidget(maxBtn);
    titleBarLayout->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);
    connect(minBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(maxBtn, &QPushButton::clicked, this, [this]() {
        if (this->isMaximized()) { this->showNormal(); } else { this->showMaximized(); }
    });

    mainLayout->addWidget(titleBar);

    auto *contentWidget = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto *leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(240);
    leftPanel->setStyleSheet("QWidget#leftPanel { border-right: 1px solid rgba(255,255,255,0.05); }");

    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 12, 0, 12);
    leftLayout->setSpacing(8);

    auto *titleLabel = new QLabel("📋 在线好友", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; padding: 4px 16px; color: rgba(255,255,255,0.6);");
    leftLayout->addWidget(titleLabel);

    m_contactList = new QListWidget(this);
    m_contactList->setObjectName("contactList");
    m_contactList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { padding: 12px 16px; border: none; border-radius: 8px; margin: 2px 12px; }"
        "QListWidget::item:hover { background: rgba(167,139,250,0.1); }"
        "QListWidget::item:selected { background: rgba(167,139,250,0.2); color: white; }"
        );
    leftLayout->addWidget(m_contactList);

    contentLayout->addWidget(leftPanel);

    auto *rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto *chatHeaderWidget = new QWidget(this);
    chatHeaderWidget->setFixedHeight(50);
    chatHeaderWidget->setStyleSheet("border-bottom: 1px solid rgba(255,255,255,0.05);");
    auto *chatHeaderLayout = new QHBoxLayout(chatHeaderWidget);
    chatHeaderLayout->setContentsMargins(20, 0, 20, 0);

    auto *chatTitleLabel = new QLabel("💬 选择好友开始聊天", this);
    chatTitleLabel->setObjectName("chatTitleLabel");
    chatTitleLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: rgba(255,255,255,0.9);");
    chatHeaderLayout->addWidget(chatTitleLabel);

    chatHeaderLayout->addStretch();

    m_videoCallBtn = new QPushButton("📹 视频通话", this);
    m_videoCallBtn->setObjectName("videoCallBtn");
    m_videoCallBtn->setFixedSize(100, 30);
    m_videoCallBtn->setVisible(false);

    m_videoCallBtn->setStyleSheet(
        "QPushButton#videoCallBtn { background-color: rgba(167,139,250,0.15); color: #a78bfa; border: 1px solid rgba(167,139,250,0.3); border-radius: 6px; font-size: 12px; }"
        "QPushButton#videoCallBtn:hover { background-color: rgba(167,139,250,0.3); }"
        );

    chatHeaderLayout->addWidget(m_videoCallBtn);
    rightLayout->addWidget(chatHeaderWidget);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("chatScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scrollArea->setStyleSheet(
        "QScrollArea#chatScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 6px; background: transparent; margin: 0px; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,0.1); border-radius: 3px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(167,139,250,0.3); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        );

    m_chatContainer = new QWidget();
    m_chatContainer->setObjectName("chatContainer");
    m_chatContainer->setStyleSheet("background: rgba(0, 0, 0, 0.05);");

    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(10);
    m_chatLayout->setContentsMargins(20, 20, 20, 20);

    m_scrollArea->setWidget(m_chatContainer);
    rightLayout->addWidget(m_scrollArea, 1);

    auto *inputContainer = new QWidget(this);
    inputContainer->setStyleSheet("background: transparent;");
    auto *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(20, 12, 20, 12);
    inputLayout->setSpacing(12);

    m_messageInput = new QLineEdit(this);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息...");
    m_messageInput->setStyleSheet(
        "QLineEdit { background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.08); "
        "border-radius: 8px; padding: 10px 16px; font-size: 14px; color: rgba(255,255,255,0.9); }"
        "QLineEdit:focus { border: 1px solid #a78bfa; background: rgba(255,255,255,0.08); }"
        );

    m_sendButton = new QPushButton("发送", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedSize(80, 38);
    m_sendButton->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a78bfa, stop:1 #8b5cf6); "
        "color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 500; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8b5cf6, stop:1 #7c3aed); }"
        "QPushButton:pressed { background: #6d28d9; }"
        );

    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addWidget(inputContainer);

    auto *statusBarWidget = new QWidget(this);
    statusBarWidget->setFixedHeight(24);
    statusBarWidget->setStyleSheet("background: rgba(0,0,0,0.1); border-top: 1px solid rgba(255,255,255,0.03);");
    auto *statusBarLayout = new QHBoxLayout(statusBarWidget);
    statusBarLayout->setContentsMargins(20, 0, 20, 0);

    m_statusLabel = new QLabel("⏳ 未连接服务器", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("color: rgba(255,255,255,0.3); font-size: 11px;");
    statusBarLayout->addWidget(m_statusLabel);
    statusBarLayout->addStretch();

    rightLayout->addWidget(statusBarWidget);

    contentLayout->addWidget(rightPanel);
    mainLayout->addWidget(contentWidget);

    setCentralWidget(central);

    connect(m_sendButton, &QPushButton::clicked, [this]() {
        QString text = m_messageInput->text().trimmed();
        if (text.isEmpty()) {
            appendMessage("系统", "消息不能为空", false);
            return;
        }
        if (!m_wsClient->isConnected()) {
            appendMessage("系统", "未连接到服务器", false);
            return;
        }
        if (m_currentContact.isEmpty()) {
            appendMessage("系统", "请先选择一个好友", false);
            return;
        }

        QJsonObject json;
        json["cmd"] = "send";
        QJsonObject data;
        data["to"] = m_currentContact;
        data["content"] = text;
        json["data"] = data;
        m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));

        appendMessage("我", text, true);
        m_messageInput->clear();
    });

    QFile styleFile("C:/my-github-project/rust/chat_app/client/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        this->setStyleSheet(style);
        qDebug() << "MainWindow 样式加载成功";
    }

    // 【主叫方】发起视频通话点击事件
    connect(m_videoCallBtn, &QPushButton::clicked, [this]() {
        if (m_currentContact.isEmpty()) return;

        m_callDialog = new VideoCallDialog(m_currentContact, true, this);
        m_callDialog->show();

        m_rtcManager = new WebRTCManager(this);

        connect(m_rtcManager, &WebRTCManager::localFrameReady, this, [this](const QImage &frame) {
            if(m_callDialog) m_callDialog->localVideoLabel()->setPixmap(QPixmap::fromImage(frame.scaled(160, 120, Qt::KeepAspectRatio)));
        });
        connect(m_rtcManager, &WebRTCManager::remoteFrameReady, this, [this](const QImage &frame) {
            if(m_callDialog) m_callDialog->remoteVideoLabel()->setPixmap(QPixmap::fromImage(frame.scaled(720, 400, Qt::KeepAspectRatio)));
        });

        connect(m_rtcManager, &WebRTCManager::sendSignaling, this, [this](const QString &type, const QString &sdp) {
            QJsonObject json;
            json["cmd"] = "video_rtc_signaling";
            QJsonObject data;
            data["to"] = m_currentContact;
            data["type"] = type;
            data["sdp"] = sdp;
            json["data"] = data;
            m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
        });

        connect(m_callDialog, &VideoCallDialog::hangUpClicked, this, [this]() {
            m_callDialog->close(); m_callDialog->deleteLater(); m_callDialog = nullptr;
            if(m_rtcManager) { m_rtcManager->deleteLater(); m_rtcManager = nullptr; }
        });

        m_rtcManager->init(true);
    });
}

void MainWindow::playNotificationSound()
{
    QApplication::beep();
}

void MainWindow::markUnread(const QString &username)
{
    for (int i = 0; i < m_contactList->count(); i++) {
        QListWidgetItem *item = m_contactList->item(i);
        if (item->text() == username) {
            item->setText(username + " 🔴");
            break;
        }
    }
}

void MainWindow::loadHistory(const QString &contact)
{
    qDebug() << "📜 加载历史消息，联系人:" << contact;

    if (!m_wsClient->isConnected()) {
        qDebug() << "❌ 未连接到服务器";
        return;
    }

    QJsonObject json;
    json["cmd"] = "history";
    QJsonObject data;
    data["to"] = contact;
    data["limit"] = 50;
    json["data"] = data;
    m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
    qDebug() << "📤 发送历史请求:" << QString::fromUtf8(QJsonDocument(json).toJson());
}

void MainWindow::appendMessage(const QString &sender, const QString &content, bool isMine)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");

    QWidget *row = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    QWidget *bubbleContainer = new QWidget();
    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubbleContainer);
    bubbleLayout->setContentsMargins(12, 8, 12, 4);
    bubbleLayout->setSpacing(2);

    QLabel *msgLabel = new QLabel(content);
    msgLabel->setWordWrap(true);
    msgLabel->setMaximumWidth(300);

    QLabel *timeLabel = new QLabel(time);
    timeLabel->setAlignment(Qt::AlignRight);
    timeLabel->setStyleSheet("color: rgba(255,255,255,0.2); font-size: 10px;");

    bubbleLayout->addWidget(msgLabel);
    bubbleLayout->addWidget(timeLabel);

    if (isMine) {
        bubbleContainer->setStyleSheet(
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            "stop:0 #a78bfa, stop:1 #8b5cf6);"
            "border-radius: 14px 14px 4px 14px;"
            );
        rowLayout->addStretch();
        rowLayout->addWidget(bubbleContainer);
    } else {
        bubbleContainer->setStyleSheet(
            "background: rgba(255,255,255,0.06);"
            "border: 1px solid rgba(255,255,255,0.04);"
            "border-radius: 14px 14px 14px 4px;"
            );
        rowLayout->addWidget(bubbleContainer);
        rowLayout->addStretch();
    }

    m_chatLayout->addWidget(row);
    m_chatLayout->addSpacing(2);

    if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
        QTimer::singleShot(10, this, [this]() {
            QScrollBar *bar = m_scrollArea->verticalScrollBar();
            bar->setValue(bar->maximum());
        });
    }
}

void MainWindow::setCurrentUser(const QString &username)
{
    static bool isConnecting = false;
    if (isConnecting) {
        qDebug() << "⚠️ 正在连接中，忽略重复请求";
        return;
    }
    isConnecting = true;

    static int callCount = 0;
    callCount++;
    qDebug() << "🔍 setCurrentUser 被调用，次数:" << callCount << ", username:" << username;

    m_currentUser = username;
    qDebug() << "✅ setCurrentUser:" << username;
    setWindowTitle(QString("Rust Chat - %1").arg(username));

    m_wsClient->connectToServer("192.168.0.103", 3000);
    m_statusLabel->setText("⏳ 正在连接服务器...");
}

void MainWindow::onConnectToServer()
{
    if (m_loginSent) {
        qDebug() << "⚠️ 登录请求已发送，忽略重复请求";
        return;
    }
    m_loginSent = true;

    qDebug() << "✅ onConnectToServer, 当前用户:" << m_currentUser;
    m_statusLabel->setText("✅ 已连接到服务器，正在登录...");

    QTimer::singleShot(500, this, [this]() {
        qDebug() << "⏰ 请求用户列表...";
        m_statusLabel->setText("✅ 登录成功，获取用户列表...");
        m_wsClient->requestUserList();
    });
}

void MainWindow::onUserListReceived(const QVariantList &users)
{
    qDebug() << "📋 收到用户列表，数量:" << users.size();
    qDebug() << "📋 当前用户:" << m_currentUser;

    if (m_currentUser.isEmpty()) {
        qDebug() << "⚠️ 当前用户为空，忽略用户列表";
        return;
    }

    m_contactList->clear();
    int addedCount = 0;
    for (const QVariant &item : users) {
        QVariantMap user = item.toMap();
        QString username = user["username"].toString();
        qDebug() << "  用户:" << username;

        if (username != m_currentUser) {
            m_contactList->addItem(username);
            addedCount++;
        }
    }
    qDebug() << "✅ 添加了" << addedCount << "个用户到列表";
    m_statusLabel->setText(QString("✅ 在线用户: %1 人").arg(m_contactList->count()));
}

void MainWindow::onContactSelected(int row)
{
    if (row < 0 || row >= m_contactList->count()) return;

    QListWidgetItem *item = m_contactList->item(row);
    QString contact = item->text();

    if (contact.contains(" 🔴")) {
        contact = contact.replace(" 🔴", "");
        item->setText(contact);
    }

    m_currentContact = contact;
    m_statusLabel->setText(QString("💬 正在与 %1 聊天").arg(contact));

    QLayoutItem *layoutItem;
    while ((layoutItem = m_chatLayout->takeAt(0)) != nullptr) {
        delete layoutItem->widget();
        delete layoutItem;
    }
    loadHistory(contact);
    appendMessage("系统", QString("开始与 %1 聊天").arg(contact), false);

    if (m_videoCallBtn) {
        m_videoCallBtn->show();
    }
}

void MainWindow::onRegisterSuccess(const QString &username)
{
    qDebug() << "📝 注册成功，自动登录用户:" << username;
    setCurrentUser(username);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "🚪 窗口关闭，发送登出请求";

    if (m_wsClient && m_wsClient->isConnected()) {
        QJsonObject json;
        json["cmd"] = "logout";
        QJsonObject data;
        data["username"] = m_currentUser;
        json["data"] = data;
        m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
        qDebug() << "📤 发送登出请求，用户:" << m_currentUser;

        QEventLoop loop;
        QTimer::singleShot(200, &loop, &QEventLoop::quit);
        loop.exec();
    }

    event->accept();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}