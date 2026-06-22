#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <memory>

class QLineEdit;
class QPushButton;
class QLabel;
class LoginController;

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(std::shared_ptr<LoginController> controller,
                         QWidget *parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginSuccess(const QVariantMap &userData);
    void onLoginFailed(const QString &error);

private:
    void setupUI();

    std::shared_ptr<LoginController> m_controller;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_nicknameEdit;      // 新增：昵称输入框
    QPushButton *m_loginButton;
    QPushButton *m_registerButton;  // 新增：注册按钮
    QLabel *m_statusLabel;
    bool m_isRegisterMode;          // 新增：是否注册模式
};

#endif