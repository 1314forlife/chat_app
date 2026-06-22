#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include <QObject>
#include <memory>

class LoginUseCase;

class LoginController : public QObject
{
    Q_OBJECT
public:
    explicit LoginController(std::shared_ptr<LoginUseCase> loginUseCase,
                             QObject *parent = nullptr);

    void handleLogin(const QString &username, const QString &password);
    void handleRegister(const QString &username, const QString &password, const QString &nickname);

signals:
    void loginSuccess(const QVariantMap &userData);
    void loginFailed(const QString &error);

private:
    std::shared_ptr<LoginUseCase> m_loginUseCase;
};

#endif