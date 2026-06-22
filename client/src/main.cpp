#include <QApplication>
#include <QDebug>
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include "application/controllers/LoginController.h"
#include "domain/usecases/LoginUseCase.h"
#include "infrastructure/network/WebSocketClient.h"
#include "infrastructure/repositories/MemoryUserRepository.h"
#include "infrastructure/repositories/MemoryMessageRepository.h"
#include "infrastructure/serialization/JsonSerializer.h"
#include "domain/entities/User.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "🚀 启动聊天客户端...";

    // 依赖注入
    auto userRepo = std::make_shared<MemoryUserRepository>();
    auto messageRepo = std::make_shared<MemoryMessageRepository>();
    auto wsClient = std::make_shared<WebSocketClient>();
    auto serializer = std::make_shared<JsonSerializer>();

    auto loginUseCase = std::make_shared<LoginUseCase>(userRepo, wsClient);
    auto loginController = std::make_shared<LoginController>(loginUseCase);

    // 创建主窗口（先不显示）
    auto mainWindow = std::make_shared<MainWindow>();

    // 创建登录对话框
    LoginDialog loginDialog(loginController);

    // 登录成功后显示主窗口
    QObject::connect(&loginDialog, &LoginDialog::accepted, [&]() {
        qDebug() << "✅ 登录成功，显示主窗口";
        mainWindow->show();
    });

    // 登录失败时重新显示登录对话框
    QObject::connect(loginController.get(), &LoginController::loginFailed, [&]() {
        qDebug() << "❌ 登录失败，重新显示登录对话框";
        loginDialog.show();
    });

    // 显示登录对话框（模态）
    loginDialog.exec();

    return app.exec();
}