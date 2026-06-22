#include "LoginDialog.h"
#include "application/controllers/LoginController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDebug>

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

    qDebug() << "✅ LoginDialog 初始化完成";
}

void LoginDialog::setupUI()
{
    setWindowTitle("登录/注册");
    setFixedSize(350, 300);

    auto *mainLayout = new QVBoxLayout(this);

    auto *titleLabel = new QLabel("🦀 Rust Chat", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);

    auto *formLayout = new QVBoxLayout();

    // 用户名
    auto *usernameLabel = new QLabel("用户名:", this);
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    formLayout->addWidget(usernameLabel);
    formLayout->addWidget(m_usernameEdit);

    // 密码
    auto *passwordLabel = new QLabel("密码:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    formLayout->addWidget(passwordLabel);
    formLayout->addWidget(m_passwordEdit);

    // 昵称（注册时显示）
    auto *nicknameLabel = new QLabel("昵称:", this);
    m_nicknameEdit = new QLineEdit(this);
    m_nicknameEdit->setPlaceholderText("请输入昵称");
    m_nicknameEdit->hide();
    nicknameLabel->hide();
    formLayout->addWidget(nicknameLabel);
    formLayout->addWidget(m_nicknameEdit);

    mainLayout->addLayout(formLayout);

    // 按钮
    auto *buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton("登录", this);
    m_registerButton = new QPushButton("注册", this);
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_registerButton);
    mainLayout->addLayout(buttonLayout);

    // 状态信息
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(m_statusLabel);

    // 切换模式提示
    auto *switchLabel = new QLabel("没有账号？点击\"注册\"创建新账号", this);
    switchLabel->setStyleSheet("color: gray; font-size: 12px;");
    mainLayout->addWidget(switchLabel, 0, Qt::AlignCenter);

    // 保存昵称标签引用
    nicknameLabel->setObjectName("nicknameLabel");

    qDebug() << "✅ UI 设置完成";
}

void LoginDialog::onLoginClicked()
{
    qDebug() << "🔐 点击登录按钮";
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("请输入用户名和密码");
        qDebug() << "❌ 用户名或密码为空";
        return;
    }

    m_statusLabel->setText("登录中...");
    qDebug() << "📤 调用登录: username=" << username;
    m_controller->handleLogin(username, password);
}

void LoginDialog::onRegisterClicked()
{
    qDebug() << "📝 点击注册按钮，当前模式:" << (m_isRegisterMode ? "注册" : "登录");

    if (!m_isRegisterMode) {
        // 切换到注册模式
        m_isRegisterMode = true;
        m_registerButton->setText("确认注册");
        m_loginButton->setEnabled(false);
        m_statusLabel->setText("请输入用户名、密码和昵称");
        findChild<QLabel*>("nicknameLabel")->show();
        m_nicknameEdit->show();
        setWindowTitle("注册新账号");
        qDebug() << "✅ 切换到注册模式";
        return;
    }

    // 确认注册
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text().trimmed();
    QString nickname = m_nicknameEdit->text().trimmed();

    qDebug() << "📝 确认注册: username=" << username << ", nickname=" << nickname;

    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("请输入用户名和密码");
        qDebug() << "❌ 用户名或密码为空";
        return;
    }

    if (nickname.isEmpty()) {
        nickname = username;  // 如果没填昵称，用用户名代替
    }

    m_statusLabel->setText("注册中...");
    qDebug() << "📤 调用注册: username=" << username << ", nickname=" << nickname;
    m_controller->handleRegister(username, password, nickname);
}

void LoginDialog::onLoginSuccess(const QVariantMap &userData)
{
    qDebug() << "✅ 登录成功:" << userData;
    m_statusLabel->setStyleSheet("color: green;");
    m_statusLabel->setText("登录成功！");
    accept();
}

void LoginDialog::onLoginFailed(const QString &error)
{
    qDebug() << "❌ 登录失败:" << error;
    m_statusLabel->setStyleSheet("color: red;");
    m_statusLabel->setText("登录失败: " + error);
}