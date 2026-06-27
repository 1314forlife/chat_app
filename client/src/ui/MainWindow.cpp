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


MainWindow::MainWindow(std::shared_ptr<LoginUseCase> loginUseCase,
                       std::shared_ptr<WebSocketClient> wsClient,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_loginUseCase(loginUseCase)
    , m_wsClient(wsClient)
{
    setupUI();

    // ============================================================
    // 断线重连信号处理
    // ============================================================
    connect(m_wsClient.get(), &WebSocketClient::reconnecting,
            this, [this]() {
                m_statusLabel->setText("🔄 连接断开，正在重连...");
                m_statusLabel->setStyleSheet("color: #fbbf24;");
                qDebug() << "🔄 断线重连中...";
            });

    connect(m_wsClient.get(), &WebSocketClient::reconnectSuccess,
            this, [this]() {
                m_statusLabel->setText("✅ 重连成功！");
                m_statusLabel->setStyleSheet("color: #34d399;");
                qDebug() << "✅ 重连成功！";

                // 重新登录
                onConnectToServer();
            });

    connect(m_wsClient.get(), &WebSocketClient::reconnectFailed,
            this, [this]() {
                m_statusLabel->setText("❌ 重连失败，请手动重启客户端");
                m_statusLabel->setStyleSheet("color: #ef4444;");
                qDebug() << "❌ 重连失败";
            });

    connect(m_wsClient.get(), &WebSocketClient::connected,
            this, &MainWindow::onConnectToServer);
    connect(m_wsClient.get(), &WebSocketClient::userListReceived,
            this, &MainWindow::onUserListReceived);

    connect(m_wsClient.get(), &WebSocketClient::messageReceived,
            this, [this](const QString &msg) {
                qDebug() << "📩 收到消息:" << msg;
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
                if (!doc.isObject()) return;

                QJsonObject obj = doc.object();

                // ============================================================
                // 1. 系统消息（上线/下线）
                // ============================================================
                if (obj.contains("from") && obj["from"].toString() == "system") {
                    QString content = obj["content"].toString();
                    QJsonDocument contentDoc = QJsonDocument::fromJson(content.toUtf8());
                    if (contentDoc.isObject()) {
                        QJsonObject contentObj = contentDoc.object();
                        if (contentObj.contains("event")) {
                            QString event = contentObj["event"].toString();
                            qDebug() << "📌 事件:" << event;

                            if (event == "user_online") {
                                QJsonObject user = contentObj["user"].toObject();
                                QString username = user["username"].toString();
                                if (username != m_currentUser) {
                                    bool exists = false;
                                    for (int i = 0; i < m_contactList->count(); i++) {
                                        if (m_contactList->item(i)->text() == username) {
                                            exists = true;
                                            break;
                                        }
                                    }
                                    if (!exists) {
                                        m_contactList->addItem(username);
                                        m_statusLabel->setText(QString("✅ %1 上线了").arg(username));
                                        qDebug() << "✅ 添加用户:" << username;
                                    }
                                }
                                return;
                            } else if (event == "user_offline") {
                                QString username = contentObj["username"].toString();
                                for (int i = 0; i < m_contactList->count(); i++) {
                                    if (m_contactList->item(i)->text() == username) {
                                        delete m_contactList->takeItem(i);
                                        m_statusLabel->setText(QString("❌ %1 下线了").arg(username));
                                        qDebug() << "✅ 移除用户:" << username;
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
                    // 检查是否是历史消息（data 是数组）
                    if (obj.contains("data") && obj["data"].isArray()) {
                        QJsonArray messages = obj["data"].toArray();

                        // 如果数组不为空，且第一个元素有 "from" 字段，说明是历史消息
                        bool isHistory = false;
                        if (!messages.isEmpty() && messages[0].isObject()) {
                            QJsonObject first = messages[0].toObject();
                            if (first.contains("from") && first.contains("content")) {
                                isHistory = true;
                            }
                        }

                        if (isHistory) {
                            qDebug() << "📜 收到历史消息，数量:" << messages.size();

                            // 清空聊天布局
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
                                    if (from == m_currentUser) {
                                        appendMessage("我", content, true);
                                    } else {
                                        appendMessage(from, content, false);
                                    }
                                }
                                qDebug() << "✅ 显示" << messages.size() << "条历史消息";
                            }
                            return;
                        }
                    }
                }

                // ============================================================
                // 2. 普通聊天消息
                // ============================================================
                if (obj.contains("from") && obj.contains("content")) {
                    QString from = obj["from"].toString();
                    QString content = obj["content"].toString();

                    if (from != "system") {
                        // ✅ 如果不是当前聊天对象，标记未读
                        if (from != m_currentContact) {
                            markUnread(from);
                        }

                        // ✅ 播放声音提示（只要不是自己发的）
                        if (from != m_currentUser) {
                            playNotificationSound();
                        }

                        // 去重
                        QString key = from + content;
                        qint64 now = QDateTime::currentMSecsSinceEpoch();
                        if (key != m_lastMessageKey || now - m_lastMessageTime > 500) {
                            // 如果是当前聊天对象，显示消息
                            if (from == m_currentContact) {
                                appendMessage(from, content, false);
                            } else {
                                // 不是当前聊天对象，只在状态栏提示
                                m_statusLabel->setText(QString("📩 来自 %1: %2").arg(from, content));
                            }
                            m_lastMessageKey = key;
                            m_lastMessageTime = now;
                        }
                    }
                    return;
                }

                // ============================================================
                // 3. 命令响应
                // ============================================================
                if (obj.contains("status") && !obj.contains("from")) {
                    qDebug() << "📩 命令响应:" << obj["status"].toString();
                    return;
                }

                qDebug() << "📩 忽略非聊天消息";
            });

    connect(m_contactList, &QListWidget::itemClicked,
            [this](QListWidgetItem *item) {
                if (item) {
                    onContactSelected(m_contactList->row(item));
                }
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

    // ============================================================
    // 1. 标题栏 (加一条淡淡的下边框区分)
    // ============================================================
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

    // ============================================================
    // 主内容
    // ============================================================
    auto *contentWidget = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // ============================================================
    // 2. 左侧 - 好友列表 (宽度 240，加右边框线)
    // ============================================================
    auto *leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(240);
    // 通过样式表加一条垂直分割线
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

    // ============================================================
    // 3. 右侧 - 整体聊天面板 (包含聊天、输入框、状态栏)
    // ============================================================
    auto *rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0); // 撑满右侧
    rightLayout->setSpacing(0);

    // 3.1 聊天顶部状态栏 (显示正在和谁聊天)
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

    rightLayout->addWidget(chatHeaderWidget);

    // 3.2 聊天消息容器 (必须加 ScrollArea，否则消息多了装不下)
    // 这里为了不破坏你原本的布局，直接用 QWidget，但给它拉伸权重
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("chatScrollArea");
    m_scrollArea->setWidgetResizable(true); // 关键：允许内部部件自适应大小
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用横向滚动条

    // 美化滚动条：让它变成极简的细条，符合整体精致的 UI 风格
    m_scrollArea->setStyleSheet(
        "QScrollArea#chatScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 6px; background: transparent; margin: 0px; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,0.1); border-radius: 3px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(167,139,250,0.3); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        );

    // 2. 创建承载消息的气泡实体容器（作为 ScrollArea 的内容主体）
    m_chatContainer = new QWidget();
    m_chatContainer->setObjectName("chatContainer");
    m_chatContainer->setStyleSheet("background: rgba(0, 0, 0, 0.05);"); // 保持你原有的深色底

    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(10);
    m_chatLayout->setContentsMargins(20, 20, 20, 20);

    // 3. 把实体容器塞进滚动外壳里
    m_scrollArea->setWidget(m_chatContainer);

    // 4. 核心：把【滚动外壳】塞进右侧主布局，并赋予拉伸权重 1
    rightLayout->addWidget(m_scrollArea, 1);

    // 3.3 底部输入区域
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

    // 3.4 最底部状态栏 (作为一个真正的底部条存在)
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

    // ============================================================
    // 信号连接与样式加载 (保持你原本的逻辑)
    // ============================================================
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

        // ✅ 发送 WebSocket 消息
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
        qDebug() << "✅ MainWindow 样式加载成功";
    }
}

// ============================================================
// ✅ 新增：播放提示音
// ============================================================
void MainWindow::playNotificationSound()
{
    // 使用系统默认提示音
    QApplication::beep();
}

// ============================================================
// ✅ 新增：标记未读
// ============================================================
void MainWindow::markUnread(const QString &username)
{
    for (int i = 0; i < m_contactList->count(); i++) {
        QListWidgetItem *item = m_contactList->item(i);
        if (item->text() == username) {
            // 添加 🔴 标记
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

// ============================================================
// appendMessage - 保留你的气泡逻辑
// ============================================================
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
        // 使用 QTimer::singleShot 是为了给 Qt 留出微秒级的时间来渲染新气泡。
        // 只有等气泡高度真正撑起来后，获取到的 maximum() 才是正确的底部高度。
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

// ============================================================
// ✅ 点击好友：清除标记，设置当前聊天对象
// ============================================================
void MainWindow::onContactSelected(int row)
{
    if (row < 0 || row >= m_contactList->count()) return;

    QListWidgetItem *item = m_contactList->item(row);
    QString contact = item->text();

    // ✅ 去掉 🔴 标记
    if (contact.contains(" 🔴")) {
        contact = contact.replace(" 🔴", "");
        item->setText(contact);
    }

    m_currentContact = contact;
    m_statusLabel->setText(QString("💬 正在与 %1 聊天").arg(contact));

    // 清空聊天消息
    QLayoutItem *layoutItem;
    while ((layoutItem = m_chatLayout->takeAt(0)) != nullptr) {
        delete layoutItem->widget();
        delete layoutItem;
    }
    loadHistory(contact);
    // 提示开始聊天
    appendMessage("系统", QString("开始与 %1 聊天").arg(contact), false);
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