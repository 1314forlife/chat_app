#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnect();
    void onSendMessage();
    void onMessageReceived();
    void onConnected();
    void onDisconnected();

private:
    void setupUI();
    void appendMessage(const QString &sender, const QString &text);

    QTcpSocket *m_socket;
    QListWidget *m_messageList;
    QLineEdit *m_inputEdit;
    QPushButton *m_sendButton;
    QPushButton *m_connectButton;
    QLabel *m_statusLabel;
};

#endif // MAINWINDOW_H