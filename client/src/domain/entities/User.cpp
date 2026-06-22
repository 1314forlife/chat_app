#include "User.h"
#include <QUuid>

User::User()
    : m_online(false)
{
}

User::User(const QString &username, const QString &nickname)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_username(username)
    , m_nickname(nickname)
    , m_online(false)
{
}

User::User(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_username = json["username"].toString();
    m_nickname = json["nickname"].toString();
    m_online = json["online"].toBool(false);
}

QJsonObject User::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["username"] = m_username;
    json["nickname"] = m_nickname;
    json["online"] = m_online;
    return json;
}

User User::fromJson(const QJsonObject &json)
{
    return User(json);
}

bool User::operator==(const User &other) const
{
    return m_id == other.m_id;
}