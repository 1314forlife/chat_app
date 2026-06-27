#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <QTimer>

typedef websocketpp::client<websocketpp::config::asio_client> WebSocketPPClient;

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void connectToServer(const QString &host, quint16 port);
    void sendMessage(const QString &message);
    void requestUserList();
    bool isConnected() const;
    void setReconnectEnabled(bool enabled);
signals:
    void connected();
    void disconnected();
    void messageReceived(const QString &message);
    void errorOccurred(const QString &error);
    void userListReceived(const QVariantList &users);

    void reconnecting();      //  正在重连
    void reconnectSuccess();  //  重连成功
    void reconnectFailed();   //  重连失败

private slots:
    void attemptReconnect();


private:
    void onMessage(const std::string &message);
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);

    WebSocketPPClient m_client;
    websocketpp::connection_hdl m_hdl;
    bool m_connected;
    QString m_host;
    quint16 m_port;

    QTimer *m_reconnectTimer = nullptr;
    int m_reconnectAttempts = 0;
    int m_maxReconnectAttempts = 5;
    bool m_reconnectEnabled = true;

};

#endif