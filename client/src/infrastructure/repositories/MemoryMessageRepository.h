#ifndef MEMORYMESSAGEREPOSITORY_H
#define MEMORYMESSAGEREPOSITORY_H

#include <QList>
#include <QMap>
#include "domain/repositories/IMessageRepository.h"
#include "domain/entities/Message.h"

class MemoryMessageRepository : public IMessageRepository
{
public:
    MemoryMessageRepository();

    void save(const Message &message) override;
    QList<Message> getHistory(const QString &user1, const QString &user2, int limit = 50) override;
    QList<Message> getMessagesForUser(const QString &username) override;
    void markAsRead(const QString &messageId) override;

private:
    QList<Message> m_messages;
    QMap<QString, int> m_messageIndex;
};

#endif