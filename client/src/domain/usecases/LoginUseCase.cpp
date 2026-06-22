#include "LoginUseCase.h"
#include "infrastructure/repositories/MemoryUserRepository.h"
#include "infrastructure/network/WebSocketClient.h"
#include "domain/entities/User.h"
#include <QDebug>

LoginUseCase::LoginUseCase(std::shared_ptr<MemoryUserRepository> userRepo,
                           std::shared_ptr<WebSocketClient> wsClient,
                           QObject *parent)
    : QObject(parent), m_userRepo(userRepo), m_wsClient(wsClient) {}

void LoginUseCase::login(const QString &username, const QString &password)
{
    qDebug() << "🔐 登录请求: username=" << username;

    // 查找用户
    auto userOpt = m_userRepo->findByUsername(username);
    if (!userOpt.has_value()) {
        qDebug() << "❌ 用户不存在:" << username;
        emit loginFailed("用户不存在");
        return;
    }

    User user = userOpt.value();
    qDebug() << "✅ 找到用户:" << user.username() << ", 昵称:" << user.nickname();

    user.setOnline(true);
    m_userRepo->save(user);

    QVariantMap userData;
    userData["username"] = user.username();
    userData["nickname"] = user.nickname();
    userData["id"] = user.id();
    qDebug() << "✅ 登录成功:" << userData;
    emit loginSuccess(userData);
}

void LoginUseCase::registerUser(const QString &username, const QString &password, const QString &nickname)
{
    qDebug() << "📝 注册请求: username=" << username << ", nickname=" << nickname;

    // 检查用户是否已存在
    auto userOpt = m_userRepo->findByUsername(username);
    if (userOpt.has_value()) {
        qDebug() << "❌ 用户名已存在:" << username;
        emit loginFailed("用户名已存在");
        return;
    }

    // 创建新用户
    User user(username, nickname.isEmpty() ? username : nickname);
    user.setOnline(true);
    m_userRepo->save(user);
    qDebug() << "✅ 用户已创建:" << user.username() << ", 昵称:" << user.nickname();

    // 连接 WebSocket
    m_wsClient->connectToServer("192.168.0.102", 3000);

    QVariantMap userData;
    userData["username"] = user.username();
    userData["nickname"] = user.nickname();
    userData["id"] = user.id();
    qDebug() << "✅ 注册成功:" << userData;
    emit loginSuccess(userData);
}