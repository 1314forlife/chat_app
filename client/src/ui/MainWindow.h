#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>
#include <QCloseEvent>

class WebSocketClient;
class LoginUseCase;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<LoginUseCase> loginUseCase,
                        std::shared_ptr<WebSocketClient> wsClient,
                        QWidget *parent = nullptr);
    ~MainWindow();

    void setCurrentUser(const QString &username);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onConnectToServer();
    void onUserListReceived(const QVariantList &users);
    void onContactSelected(int row);
    void onRegisterSuccess(const QString &username);

private:
    void setupUI();
    void appendMessage(const QString &sender, const QString &content);

    std::shared_ptr<LoginUseCase> m_loginUseCase;
    std::shared_ptr<WebSocketClient> m_wsClient;
    QString m_currentUser;

    bool m_loginSent = false;

    QListWidget *m_contactList = nullptr;
    QTextEdit *m_chatDisplay = nullptr;
    QLineEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif