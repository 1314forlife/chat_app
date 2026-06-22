#include "Message.h"
#include <QUuid>

Message::Message()
    : m_isRead(false)
{
}

Message::Message(const QString &from, const QString &to, const QString &content)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_from(from)
    , m_to(to)
    , m_content(content)
    , m_timestamp(QDateTime::currentDateTime())
    , m_isRead(false)
{
}

Message::Message(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_from = json["from"].toString();
    m_to = json["to"].toString();
    m_content = json["content"].toString();
    m_timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    m_isRead = json["isRead"].toBool(false);
}

QJsonObject Message::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["from"] = m_from;
    json["to"] = m_to;
    json["content"] = m_content;
    json["timestamp"] = m_timestamp.toString(Qt::ISODate);
    json["isRead"] = m_isRead;
    return json;
}

Message Message::fromJson(const QJsonObject &json)
{
    return Message(json);
}