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
#include <QMouseEvent>
#include <QPoint>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <ui/VideoCallDialog.h>
#include <infrastructure/media/WebRTCManager.h>

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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onConnectToServer();
    void onUserListReceived(const QVariantList &users);
    void onContactSelected(int row);
    void onRegisterSuccess(const QString &username);
    // ✅ 这里已经干净了，没有任何变量定义

private:
    void setupUI();
    void appendMessage(const QString &sender, const QString &content, bool isMine = false);
    void playNotificationSound();
    void markUnread(const QString &username);
    void loadHistory(const QString &contact);

    std::shared_ptr<LoginUseCase> m_loginUseCase;
    std::shared_ptr<WebSocketClient> m_wsClient;
    QString m_currentUser;
    QString m_currentContact;

    QPushButton      *m_videoCallBtn = nullptr; // 视频按钮
    VideoCallDialog  *m_callDialog = nullptr;   // 视频弹窗
    WebRTCManager    *m_rtcManager = nullptr;   // RTC管理器

    bool m_loginSent = false;
    bool m_dragging = false;
    QPoint m_dragStartPos;

    // UI 组件
    QListWidget *m_contactList = nullptr;
    QWidget *m_chatContainer = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    QLineEdit *m_messageInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QLabel *m_statusLabel = nullptr;

    // 去重
    QString m_lastMessageKey;
    qint64 m_lastMessageTime = 0;

    QScrollArea *m_scrollArea = nullptr;
};

#endif