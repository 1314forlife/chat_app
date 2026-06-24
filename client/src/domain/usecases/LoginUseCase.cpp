#include "LoginUseCase.h"
#include "infrastructure/repositories/MemoryUserRepository.h"
#include "infrastructure/network/WebSocketClient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

LoginUseCase::LoginUseCase(std::shared_ptr<MemoryUserRepository> userRepo,
                           std::shared_ptr<WebSocketClient> wsClient,
                           QObject *parent)
    : QObject(parent), m_userRepo(userRepo), m_wsClient(wsClient)
{
    connect(m_wsClient.get(), &WebSocketClient::connected,
            this, &LoginUseCase::onWebSocketConnected);
    connect(m_wsClient.get(), &WebSocketClient::messageReceived,
            this, &LoginUseCase::onMessageReceived);
}

void LoginUseCase::login(const QString &username, const QString &password)
{
    qDebug() << "🔐 登录请求: username=" << username;
    m_processed = false;
    m_pendingUsername = username;
    m_pendingNickname = username;
    m_isRegisterMode = false;
    m_wsClient->connectToServer("192.168.0.102", 3000);
}

void LoginUseCase::registerUser(const QString &username, const QString &password, const QString &nickname)
{
    qDebug() << "📝 注册请求: username=" << username << ", nickname=" << nickname;
    m_processed = false;
    m_pendingUsername = username;
    m_pendingNickname = nickname.isEmpty() ? username : nickname;
    m_isRegisterMode = true;
    m_wsClient->connectToServer("192.168.0.102", 3000);
}

void LoginUseCase::onWebSocketConnected()
{
    qDebug() << "✅ WebSocket 已连接，发送" << (m_isRegisterMode ? "注册" : "登录") << "请求";

    QJsonObject json;
    json["cmd"] = m_isRegisterMode ? "register" : "login";
    QJsonObject data;
    data["username"] = m_pendingUsername;
    data["nickname"] = m_pendingNickname;
    json["data"] = data;

    m_wsClient->sendMessage(QString::fromUtf8(QJsonDocument(json).toJson()));
}

void LoginUseCase::onMessageReceived(const QString &message)
{
    if (m_processed) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();

    // ✅ 只处理有 status 的消息（登录/注册响应）
    if (!obj.contains("status")) {
        return;
    }

    QString status = obj["status"].toString();

    if (status == "success") {
        m_processed = true;
        QJsonObject data = obj["data"].toObject();
        QVariantMap userData;
        userData["username"] = data["username"].toString();
        userData["nickname"] = data["nickname"].toString();
        userData["id"] = data["id"].toString();

        if (m_isRegisterMode) {
            emit registerSuccess(m_pendingUsername);
        } else {
            emit loginSuccess(userData);
        }
    } else {
        m_processed = true;
        QString errorMsg = obj["message"].toString();
        emit loginFailed(errorMsg);
    }
}