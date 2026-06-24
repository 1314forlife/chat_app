#include "WebSocketClient.h"
#include <QDebug>
#include <thread>
#include <chrono>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_port(3000)
{
    try {
        m_client.clear_access_channels(websocketpp::log::alevel::all);
        m_client.clear_error_channels(websocketpp::log::elevel::all);

        m_client.init_asio();

        m_client.set_open_handler(std::bind(&WebSocketClient::onOpen, this, std::placeholders::_1));
        m_client.set_close_handler(std::bind(&WebSocketClient::onClose, this, std::placeholders::_1));
        m_client.set_fail_handler(std::bind(&WebSocketClient::onFail, this, std::placeholders::_1));

        m_client.set_message_handler([this](websocketpp::connection_hdl, WebSocketPPClient::message_ptr msg) {
            onMessage(msg->get_payload());
        });

        qDebug() << "✅ WebSocketClient 初始化完成";
    } catch (const std::exception &e) {
        qDebug() << "❌ WebSocketClient 初始化失败:" << e.what();
    }
}

WebSocketClient::~WebSocketClient()
{
    if (m_connected) {
        try {
            m_client.close(m_hdl, websocketpp::close::status::normal, "");
        } catch (...) {}
    }
    m_client.stop();
}

void WebSocketClient::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_connected = false;

    qDebug() << "🔌 连接 WebSocket 服务器:" << host << port;

    try {
        std::string uri = "ws://" + host.toStdString() + ":" + std::to_string(port) + "/ws";
        websocketpp::lib::error_code ec;
        WebSocketPPClient::connection_ptr con = m_client.get_connection(uri, ec);

        if (ec) {
            qDebug() << "❌ 连接失败:" << ec.message().c_str();
            emit errorOccurred(QString::fromStdString(ec.message()));
            return;
        }

        m_hdl = con->get_handle();
        m_client.connect(con);

        std::thread([this]() {
            try {
                m_client.run();
            } catch (const std::exception &e) {
                qDebug() << "❌ WebSocket 运行错误:" << e.what();
            }
        }).detach();

    } catch (const std::exception &e) {
        qDebug() << "❌ 连接异常:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

bool WebSocketClient::isConnected() const
{
    return m_connected;
}

void WebSocketClient::sendMessage(const QString &message)
{
    if (!m_connected) {
        qDebug() << "❌ 未连接到服务器";
        return;
    }

    try {
        websocketpp::lib::error_code ec;
        m_client.send(m_hdl, message.toStdString(), websocketpp::frame::opcode::text, ec);
        if (ec) {
            qDebug() << "❌ 发送失败:" << ec.message().c_str();
        } else {
            qDebug() << "📤 发送:" << message;
        }
    } catch (const std::exception &e) {
        qDebug() << "❌ 发送异常:" << e.what();
    }
}

void WebSocketClient::requestUserList()
{
    QJsonObject json;
    json["cmd"] = "users";
    json["data"] = QJsonObject();
    sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
}

void WebSocketClient::onMessage(const std::string &message)
{
    QString msg = QString::fromStdString(message);
    qDebug() << "📩 收到原始消息:" << msg;

    // ✅ 先转发所有消息给 MainWindow
    emit messageReceived(msg);

    // 解析用户列表
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString status = obj["status"].toString();

        if (status == "success") {
            QJsonObject data = obj["data"].toObject();
            if (data.contains("users")) {
                QJsonArray usersArray = data["users"].toArray();
                QVariantList users;
                for (const QJsonValue &val : usersArray) {
                    users.append(val.toObject().toVariantMap());
                }
                emit userListReceived(users);
                qDebug() << "📋 收到" << users.size() << "个在线用户";
                return;
            }
        }
    }
}

void WebSocketClient::onOpen(websocketpp::connection_hdl hdl)
{
    m_connected = true;
    qDebug() << "✅ WebSocket 连接成功！";
    emit connected();
}

void WebSocketClient::onClose(websocketpp::connection_hdl hdl)
{
    m_connected = false;
    qDebug() << "❌ WebSocket 断开连接";
    emit disconnected();
}

void WebSocketClient::onFail(websocketpp::connection_hdl hdl)
{
    m_connected = false;
    qDebug() << "❌ WebSocket 连接失败";
    emit errorOccurred("WebSocket 连接失败");
}