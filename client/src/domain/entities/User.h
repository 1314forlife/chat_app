#ifndef USER_H
#define USER_H

#include <QString>
#include <QJsonObject>
#include <QJsonValue>

class User
{
public:
    User();
    User(const QString &username, const QString &nickname);
    User(const QJsonObject &json);

    // Getters
    QString id() const { return m_id; }
    QString username() const { return m_username; }
    QString nickname() const { return m_nickname; }
    bool isOnline() const { return m_online; }

    // Setters
    void setId(const QString &id) { m_id = id; }
    void setUsername(const QString &username) { m_username = username; }
    void setNickname(const QString &nickname) { m_nickname = nickname; }
    void setOnline(bool online) { m_online = online; }

    // 序列化
    QJsonObject toJson() const;
    static User fromJson(const QJsonObject &json);

    // 比较
    bool operator==(const User &other) const;
    bool operator!=(const User &other) const { return !(*this == other); }

private:
    QString m_id;
    QString m_username;
    QString m_nickname;
    bool m_online;
};

#endif // USER_H