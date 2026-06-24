#include "LoginDialog.h"
#include "application/controllers/LoginController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDebug>
#include <QFile>

LoginDialog::LoginDialog(std::shared_ptr<LoginController> controller, QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_isRegisterMode(false)
{
    setupUI();

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_controller.get(), &LoginController::loginSuccess,
            this, &LoginDialog::onLoginSuccess);
    connect(m_controller.get(), &LoginController::loginFailed,
            this, &LoginDialog::onLoginFailed);
    connect(m_controller.get(), &LoginController::registerSuccess,
            this, &LoginDialog::onRegisterSuccess);

    qDebug() << "✅ LoginDialog 初始化完成";
}

void LoginDialog::setupUI()
{
    // 无边框，不透明
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setFixedSize(380, 440);

    // ✅ 深色渐变背景（不透明）
    QString style = R"(
        LoginDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0f0c29,
                stop:0.5 #1a1a3e,
                stop:1 #24243e
            );
            border-radius: 16px;
            border: 1px solid rgba(255,255,255,0.05);
        }
        LoginDialog QLabel#titleLabel {
            color: rgba(255,255,255,0.9);
            font-size: 28px;
            font-weight: 300;
            letter-spacing: 4px;
        }
        LoginDialog QLabel[fieldLabel="true"] {
            color: rgba(255,255,255,0.4);
            font-size: 13px;
        }
        LoginDialog QLineEdit {
            background: rgba(255,255,255,0.06);
            border: 1px solid rgba(255,255,255,0.06);
            border-radius: 10px;
            padding: 10px 14px;
            font-size: 14px;
            color: rgba(255,255,255,0.85);
            min-height: 24px;
        }
        LoginDialog QLineEdit:focus {
            border: 1px solid rgba(167,139,250,0.3);
            background: rgba(255,255,255,0.08);
        }
        LoginDialog QLineEdit::placeholder {
            color: rgba(255,255,255,0.2);
            font-size: 13px;
        }
        LoginDialog QPushButton {
            border: none;
            border-radius: 10px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: 500;
            min-height: 28px;
            letter-spacing: 2px;
        }
        LoginDialog QPushButton#loginButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #a78bfa,
                stop:1 #8b5cf6);
            color: white;
        }
        LoginDialog QPushButton#loginButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #8b5cf6,
                stop:1 #7c3aed);
        }
        LoginDialog QPushButton#registerButton {
            background: rgba(255,255,255,0.06);
            color: rgba(255,255,255,0.5);
            border: 1px solid rgba(255,255,255,0.06);
        }
        LoginDialog QPushButton#registerButton:hover {
            background: rgba(255,255,255,0.1);
            color: rgba(255,255,255,0.7);
        }
        LoginDialog QLabel#statusLabel {
            color: #f472b6;
            font-size: 13px;
            padding: 4px 0;
        }
        LoginDialog QLabel#switchLabel {
            color: rgba(255,255,255,0.15);
            font-size: 12px;
            letter-spacing: 1px;
        }
    )";

    this->setStyleSheet(style);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(12);

    // 标题
    auto *titleLabel = new QLabel("🦀 Rust Chat", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(8);

    // 表单
    auto *formLayout = new QVBoxLayout();
    formLayout->setSpacing(8);

    auto *usernameLabel = new QLabel("用户名", this);
    usernameLabel->setProperty("fieldLabel", true);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    formLayout->addWidget(usernameLabel);
    formLayout->addWidget(m_usernameEdit);

    auto *passwordLabel = new QLabel("密码", this);
    passwordLabel->setProperty("fieldLabel", true);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    formLayout->addWidget(passwordLabel);
    formLayout->addWidget(m_passwordEdit);

    auto *nicknameLabel = new QLabel("昵称", this);
    nicknameLabel->setProperty("fieldLabel", true);
    m_nicknameEdit = new QLineEdit(this);
    m_nicknameEdit->setPlaceholderText("请输入昵称");
    m_nicknameEdit->hide();
    nicknameLabel->hide();
    formLayout->addWidget(nicknameLabel);
    formLayout->addWidget(m_nicknameEdit);

    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(8);

    // 按钮
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    m_loginButton = new QPushButton("登录", this);
    m_loginButton->setObjectName("loginButton");
    m_registerButton = new QPushButton("注册", this);
    m_registerButton->setObjectName("registerButton");
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_registerButton);
    mainLayout->addLayout(buttonLayout);

    // 状态信息
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(m_statusLabel);

    // 切换提示
    auto *switchLabel = new QLabel("没有账号？点击\"注册\"创建新账号", this);
    switchLabel->setObjectName("switchLabel");
    switchLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(switchLabel);
}

void LoginDialog::onLoginClicked()
{
    qDebug() << "🔐 点击登录按钮";
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("请输入用户名和密码");
        return;
    }

    m_statusLabel->setText("登录中...");
    m_controller->handleLogin(username, password);
}

void LoginDialog::onRegisterClicked()
{
    if (!m_isRegisterMode) {
        m_isRegisterMode = true;
        m_registerButton->setText("确认注册");
        m_loginButton->setEnabled(false);
        m_nicknameEdit->show();
        // 显示昵称标签
        auto *nicknameLabel = findChild<QLabel*>("fieldLabel");
        if (nicknameLabel) nicknameLabel->show();
        setWindowTitle("注册新账号");
        m_statusLabel->setText("请输入用户名、密码和昵称");
        return;
    }

    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();
    QString nickname = m_nicknameEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("请输入用户名和密码");
        return;
    }

    if (nickname.isEmpty()) {
        nickname = username;
    }

    m_statusLabel->setText("注册中...");
    m_controller->handleRegister(username, password, nickname);
}

void LoginDialog::onRegisterSuccess(const QString &username)
{
    qDebug() << "✅ 注册成功:" << username;
    m_statusLabel->setText(QString("注册成功！用户 %1 已创建，请登录").arg(username));

    m_isRegisterMode = false;
    m_registerButton->setText("注册");
    m_loginButton->setEnabled(true);
    m_nicknameEdit->hide();
    setWindowTitle("登录/注册");

    m_passwordEdit->clear();
    m_usernameEdit->setText(username);
}

void LoginDialog::onLoginSuccess(const QVariantMap &userData)
{
    qDebug() << "✅ 登录成功:" << userData;
    static bool alreadyAccepted = false;
    if (alreadyAccepted) return;
    alreadyAccepted = true;
    accept();
}

void LoginDialog::onLoginFailed(const QString &error)
{
    qDebug() << "❌ 登录失败:" << error;
    m_statusLabel->setText("登录失败: " + error);

    if (m_isRegisterMode) {
        m_isRegisterMode = false;
        m_registerButton->setText("注册");
        m_loginButton->setEnabled(true);
        m_nicknameEdit->hide();
        setWindowTitle("登录/注册");
    }
}