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
#include <QScrollBar>

MainWindow::MainWindow(std::shared_ptr<LoginUseCase> loginUseCase,
                       std::shared_ptr<WebSocketClient> wsClient,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_loginUseCase(loginUseCase)
    , m_wsClient(wsClient)
{
    setupUI();

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

                // 检查是否是系统消息（from == "system"）
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
                                return;  // ✅ 不显示在聊天框
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
                                return;  // ✅ 不显示在聊天框
                            }
                        }
                    }
                    return;  // ✅ 系统消息不显示在聊天框
                }

                // ✅ 只有普通聊天消息才显示
                // 检查是否是聊天消息（有 from 和 content）
                if (obj.contains("status") && !obj.contains("from")) {
                    qDebug() << "📩 忽略命令响应";
                    return;
                }

                // ✅ 只有普通聊天消息才显示（有 from 和 content）
                if (obj.contains("from") && obj.contains("content")) {
                    QString from = obj["from"].toString();
                    QString content = obj["content"].toString();

                    if (from != "system" && from != m_currentUser) {
                        // ✅ 去重：防止重复消息
                        QString key = from + content;
                        qint64 now = QDateTime::currentMSecsSinceEpoch();
                        if (key != m_lastMessageKey || now - m_lastMessageTime > 500) {
                            appendMessage(from, content, false);
                            m_lastMessageKey = key;
                            m_lastMessageTime = now;
                        } else {
                            qDebug() << "📩 忽略重复消息:" << content;
                        }
                    }
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
    // 无边框
    setWindowFlags(Qt::FramelessWindowHint);

    setWindowTitle("Rust Chat");
    resize(900, 600);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ============================================================
    // ✅ 自定义标题栏
    // ============================================================
    auto *titleBar = new QWidget(this);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(44);

    auto *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(16, 0, 12, 0);
    titleBarLayout->setSpacing(6);

    auto *titleText = new QLabel("✦ Rust Chat", this);
    titleText->setObjectName("titleText");

    auto *minBtn = new QPushButton("─", this);
    auto *maxBtn = new QPushButton("☐", this);
    auto *closeBtn = new QPushButton("✕", this);
    minBtn->setObjectName("minBtn");
    maxBtn->setObjectName("maxBtn");
    closeBtn->setObjectName("closeBtn");

    minBtn->setFixedSize(28, 24);
    maxBtn->setFixedSize(28, 24);
    closeBtn->setFixedSize(28, 24);

    titleBarLayout->addWidget(titleText);
    titleBarLayout->addStretch();
    titleBarLayout->addWidget(minBtn);
    titleBarLayout->addWidget(maxBtn);
    titleBarLayout->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);
    connect(minBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(maxBtn, &QPushButton::clicked, this, [this]() {
        if (this->isMaximized()) {
            this->showNormal();
        } else {
            this->showMaximized();
        }
    });

    mainLayout->addWidget(titleBar);

    // ============================================================
    // 主内容
    // ============================================================
    auto *contentWidget = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(250);
    auto *leftLayout = new QVBoxLayout(leftPanel);

    auto *titleLabel = new QLabel("📋 在线好友", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(titleLabel);

    m_contactList = new QListWidget(this);
    m_contactList->setStyleSheet("QListWidget::item { padding: 8px; }");
    leftLayout->addWidget(m_contactList);

    contentLayout->addWidget(leftPanel);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);

    m_chatContainer = new QWidget(this);
    m_chatContainer->setStyleSheet("background: transparent;");
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setAlignment(Qt::AlignTop);
    m_chatLayout->setSpacing(6);
    m_chatLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(m_chatContainer);

    auto *inputLayout = new QHBoxLayout();
    m_messageInput = new QLineEdit(this);
    m_messageInput->setPlaceholderText("输入消息...");
    m_sendButton = new QPushButton("发送", this);
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

    m_statusLabel = new QLabel("⏳ 未连接服务器", this);
    rightLayout->addWidget(m_statusLabel);

    contentLayout->addWidget(rightPanel);

    mainLayout->addWidget(contentWidget);

    setCentralWidget(central);

    connect(m_sendButton, &QPushButton::clicked, [this]() {
        QString text = m_messageInput->text().trimmed();
        if (!text.isEmpty() && m_wsClient->isConnected()) {
            QJsonObject json;
            json["cmd"] = "send";
            QJsonObject data;
            data["to"] = "bob";
            data["content"] = text;
            json["data"] = data;
            m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));

            appendMessage("我", text, true);  // ✅ 只调用一次
            m_messageInput->clear();
        }
    });

    // 加载样式
    QFile styleFile("C:/my-github-project/rust/chat_app/client/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        this->setStyleSheet(style);
        qDebug() << "✅ MainWindow 样式加载成功";
    } else {
        qDebug() << "❌ MainWindow 样式加载失败";
    }
}

void MainWindow::appendMessage(const QString &sender, const QString &content, bool isMine)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");

    QWidget *row = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    // 气泡容器
    QWidget *bubbleContainer = new QWidget();
    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubbleContainer);
    bubbleLayout->setContentsMargins(12, 8, 12, 4);
    bubbleLayout->setSpacing(2);

    // 消息文字
    QLabel *msgLabel = new QLabel(content);
    msgLabel->setWordWrap(true);
    msgLabel->setMaximumWidth(300);

    // 时间戳
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

    // ✅ 不再在这里发送 login，由 LoginUseCase 发送
    // 但为了兼容，保留空实现
    // 实际 login/register 已经在 LoginUseCase::onWebSocketConnected 中发送

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

    // ✅ 如果当前用户为空，说明还没登录完成，忽略这次列表更新
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
    QString contact = m_contactList->item(row)->text();
    m_statusLabel->setText(QString("💬 正在与 %1 聊天").arg(contact));

    // ✅ 清空聊天消息（用 m_chatLayout）
    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
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
        data["username"] = m_currentUser;  // ✅ 携带用户名
        json["data"] = data;
        m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
        qDebug() << "📤 发送登出请求，用户:" << m_currentUser;

        // 等待消息发送完成
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