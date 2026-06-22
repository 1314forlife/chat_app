#include "MainWindow.h"
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Rust Chat");
    resize(800, 600);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *label = new QLabel("欢迎来到 Rust Chat！", this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 24px;");
    layout->addWidget(label);

    auto *info = new QLabel("主窗口开发中...", this);
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    setCentralWidget(central);
}

MainWindow::~MainWindow() {}

void MainWindow::setCurrentUser(const QVariantMap &user)
{
    setWindowTitle(QString("Rust Chat - %1").arg(user["username"].toString()));
}