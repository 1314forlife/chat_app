#include "LoginController.h"
#include "domain/usecases/LoginUseCase.h"

LoginController::LoginController(std::shared_ptr<LoginUseCase> loginUseCase,
                                 QObject *parent)
    : QObject(parent), m_loginUseCase(loginUseCase) {}

void LoginController::handleLogin(const QString &username, const QString &password)
{
    connect(m_loginUseCase.get(), &LoginUseCase::loginSuccess,
            this, &LoginController::loginSuccess);
    connect(m_loginUseCase.get(), &LoginUseCase::loginFailed,
            this, &LoginController::loginFailed);
    m_loginUseCase->login(username, password);
}

void LoginController::handleRegister(const QString &username, const QString &password, const QString &nickname)
{
    connect(m_loginUseCase.get(), &LoginUseCase::loginSuccess,
            this, &LoginController::loginSuccess);
    connect(m_loginUseCase.get(), &LoginUseCase::loginFailed,
            this, &LoginController::loginFailed);
    m_loginUseCase->registerUser(username, password, nickname);
}