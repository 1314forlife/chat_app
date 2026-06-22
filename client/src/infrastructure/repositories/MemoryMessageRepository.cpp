#include "MemoryMessageRepository.h"
#include <QDebug>

MemoryMessageRepository::MemoryMessageRepository() {}

void MemoryMessageRepository::save(const Message &message)
{
    qDebug() << "💾 保存消息: id=" << message.id() << ", from=" << message.from() << ", to=" << message.to();
    m_messages.append(message);
    m_messageIndex[message.id()] = m_messages.size() - 1;
}

QList<Message> MemoryMessageRepository::getHistory(const QString &user1, const QString &user2, int limit)
{
    qDebug() << "📜 获取历史: user1=" << user1 << ", user2=" << user2 << ", limit=" << limit;
    QList<Message> result;
    for (int i = m_messages.size() - 1; i >= 0 && result.size() < limit; --i) {
        const Message &msg = m_messages[i];
        if ((msg.from() == user1 && msg.to() == user2) ||
            (msg.from() == user2 && msg.to() == user1)) {
            result.prepend(msg);
        }
    }
    qDebug() << "   📊 找到" << result.size() << "条消息";
    return result;
}

QList<Message> MemoryMessageRepository::getMessagesForUser(const QString &username)
{
    QList<Message> result;
    for (const Message &msg : m_messages) {
        if (msg.from() == username || msg.to() == username) {
            result.append(msg);
        }
    }
    return result;
}

void MemoryMessageRepository::markAsRead(const QString &messageId)
{
    if (!m_messageIndex.contains(messageId)) return;
    int index = m_messageIndex[messageId];
    if (index >= 0 && index < m_messages.size()) {
        Message msg = m_messages[index];
        msg.setRead(true);
        m_messages[index] = msg;
    }
}