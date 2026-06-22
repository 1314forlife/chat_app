#include "WebSocketClient.h"
#include <QHostAddress>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &WebSocketClient::onReadyRead);
}

void WebSocketClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void WebSocketClient::sendMessage(const QString &message)
{
    m_socket->write(message.toUtf8());
    m_socket->flush();
}

void WebSocketClient::onConnected()
{
    emit connected();
}

void WebSocketClient::onDisconnected()
{
    emit disconnected();
}

void WebSocketClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    m_buffer += QString::fromUtf8(data);
    emit messageReceived(m_buffer);
}