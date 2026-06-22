#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

class Message
{
public:
    Message();
    Message(const QString &from, const QString &to, const QString &content);
    Message(const QJsonObject &json);

    // Getters
    QString id() const { return m_id; }
    QString from() const { return m_from; }
    QString to() const { return m_to; }
    QString content() const { return m_content; }
    QDateTime timestamp() const { return m_timestamp; }
    bool isRead() const { return m_isRead; }

    // Setters
    void setId(const QString &id) { m_id = id; }
    void setFrom(const QString &from) { m_from = from; }
    void setTo(const QString &to) { m_to = to; }
    void setContent(const QString &content) { m_content = content; }
    void setTimestamp(const QDateTime &timestamp) { m_timestamp = timestamp; }
    void setRead(bool read) { m_isRead = read; }

    // 序列化
    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject &json);

    // 判断消息方向
    bool isFromMe(const QString &myUsername) const { return m_from == myUsername; }

private:
    QString m_id;
    QString m_from;
    QString m_to;
    QString m_content;
    QDateTime m_timestamp;
    bool m_isRead;
};

#endif // MESSAGE_H