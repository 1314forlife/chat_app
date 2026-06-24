#include <QApplication>
#include <QDebug>
#include <memory>
#include <functional>
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "application/controllers/LoginController.h"
#include "domain/usecases/LoginUseCase.h"
#include "infrastructure/network/WebSocketClient.h"
#include "infrastructure/repositories/MemoryUserRepository.h"
#include "infrastructure/repositories/MemoryMessageRepository.h"
#include "infrastructure/serialization/JsonSerializer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "🚀 启动聊天客户端...";

    auto userRepo = std::make_shared<MemoryUserRepository>();
    auto messageRepo = std::make_shared<MemoryMessageRepository>();
    auto wsClient = std::make_shared<WebSocketClient>();
    auto serializer = std::make_shared<JsonSerializer>();

    auto loginUseCase = std::make_shared<LoginUseCase>(userRepo, wsClient);
    auto loginController = std::make_shared<LoginController>(loginUseCase);

    auto mainWindow = std::make_shared<MainWindow>(loginUseCase, wsClient);

    LoginDialog loginDialog(loginController);

    QObject::connect(loginController.get(), &LoginController::loginSuccess,
                     std::function<void(const QVariantMap&)>([&](const QVariantMap &userData) {
                         QString username = userData["username"].toString();
                         qDebug() << "✅ 登录成功，设置当前用户:" << username;
                         mainWindow->setCurrentUser(username);
                         mainWindow->show();
                         loginDialog.accept();
                     })
                     );

    QObject::connect(loginController.get(), &LoginController::loginFailed,
                     std::function<void(const QString&)>([&](const QString &error) {
                         qDebug() << "❌ 登录失败:" << error;
                         loginDialog.show();
                     })
                     );

    loginDialog.exec();

    return app.exec();
}