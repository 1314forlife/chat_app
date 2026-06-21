#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_socket(new QTcpSocket(this))
{
    setupUI();

    connect(m_socket, &QTcpSocket::connected,
            this, &MainWindow::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &MainWindow::onMessageReceived);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    m_messageList = new QListWidget(this);
    layout->addWidget(m_messageList);

    auto *inputLayout = new QHBoxLayout();
    m_inputEdit = new QLineEdit(this);
    m_sendButton = new QPushButton("发送", this);
    inputLayout->addWidget(m_inputEdit);
    inputLayout->addWidget(m_sendButton);
    layout->addLayout(inputLayout);

    auto *statusLayout = new QHBoxLayout();
    m_connectButton = new QPushButton("连接服务器", this);
    m_statusLabel = new QLabel("未连接", this);
    statusLayout->addWidget(m_connectButton);
    statusLayout->addWidget(m_statusLabel);
    layout->addLayout(statusLayout);

    setCentralWidget(central);
    setWindowTitle("Chat Client");

    connect(m_sendButton, &QPushButton::clicked,
            this, &MainWindow::onSendMessage);
    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnect);
}

void MainWindow::onConnect()
{
    m_socket->connectToHost(QHostAddress("8.134.191.125"), 3000);
    m_statusLabel->setText("连接中...");
}

void MainWindow::onConnected()
{
    m_statusLabel->setText("● 已连接");
    m_connectButton->setEnabled(false);
    appendMessage("系统", "已连接到服务器");
}

void MainWindow::onSendMessage()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) return;

    appendMessage("我", text);

    QJsonObject msg;
    msg["type"] = "chat";
    msg["content"] = text;
    m_socket->write(QJsonDocument(msg).toJson());
    m_socket->flush();

    m_inputEdit->clear();
}

void MainWindow::onMessageReceived()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString content = obj["content"].toString();
        appendMessage("服务器", content);
    }
}

void MainWindow::appendMessage(const QString &sender, const QString &text)
{
    m_messageList->addItem(QString("[%1] %2").arg(sender, text));
    m_messageList->scrollToBottom();
}

void MainWindow::onDisconnected()
{
    m_statusLabel->setText("已断开");
    m_connectButton->setEnabled(true);
    appendMessage("系统", "与服务器断开连接");
}