#ifndef LOGINUSECASE_H
#define LOGINUSECASE_H

#include <QObject>
#include <memory>

class MemoryUserRepository;
class WebSocketClient;

class LoginUseCase : public QObject
{
    Q_OBJECT
public:
    explicit LoginUseCase(std::shared_ptr<MemoryUserRepository> userRepo,
                          std::shared_ptr<WebSocketClient> wsClient,
                          QObject *parent = nullptr);

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password, const QString &nickname);

signals:
    void loginSuccess(const QVariantMap &userData);
    void loginFailed(const QString &error);
    void registerSuccess(const QString &username);  // ✅ 新增：注册成功信号

private slots:
    void onWebSocketConnected();
    void onMessageReceived(const QString &message);

private:
    std::shared_ptr<MemoryUserRepository> m_userRepo;
    std::shared_ptr<WebSocketClient> m_wsClient;

    QString m_pendingUsername;
    QString m_pendingNickname;
    bool m_isRegisterMode = false;
    bool m_processed = false;
};

#endif