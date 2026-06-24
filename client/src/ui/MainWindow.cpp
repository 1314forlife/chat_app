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

                appendMessage("服务器", msg);
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
    QFile styleFile("C:/my-github-project/rust/chat_app/client/resources/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        this->setStyleSheet(style);
        qDebug() << "✅ MainWindow 样式加载成功";
    } else {
        qDebug() << "❌ MainWindow 样式加载失败";
    }
    setWindowTitle("Rust Chat");
    resize(900, 600);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);

    auto *leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(250);
    auto *leftLayout = new QVBoxLayout(leftPanel);

    auto *titleLabel = new QLabel("📋 在线好友", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(titleLabel);

    m_contactList = new QListWidget(this);
    m_contactList->setStyleSheet("QListWidget::item { padding: 8px; }");
    leftLayout->addWidget(m_contactList);

    mainLayout->addWidget(leftPanel);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);

    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setStyleSheet("font-size: 14px;");
    rightLayout->addWidget(m_chatDisplay);

    auto *inputLayout = new QHBoxLayout();
    m_messageInput = new QLineEdit(this);
    m_messageInput->setPlaceholderText("输入消息...");
    m_sendButton = new QPushButton("发送", this);
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);
    rightLayout->addLayout(inputLayout);

    m_statusLabel = new QLabel("⏳ 未连接服务器", this);
    rightLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(rightPanel);

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
            appendMessage("我", text);
            m_messageInput->clear();
        }
    });
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

    m_wsClient->connectToServer("192.168.0.102", 3000);
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

void MainWindow::appendMessage(const QString &sender, const QString &content)
{
    QString time = QDateTime::currentDateTime().toString("hh:mm");
    m_chatDisplay->append(QString("[%1] %2: %3").arg(time, sender, content));
}

void MainWindow::onContactSelected(int row)
{
    if (row < 0 || row >= m_contactList->count()) return;
    QString contact = m_contactList->item(row)->text();
    m_statusLabel->setText(QString("💬 正在与 %1 聊天").arg(contact));
    m_chatDisplay->append(QString("📞 开始与 %1 聊天...").arg(contact));
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