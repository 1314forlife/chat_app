#ifndef IMESSAGEREPOSITORY_H
#define IMESSAGEREPOSITORY_H

#include <QString>
#include <QList>
#include "domain/entities/Message.h"

class IMessageRepository
{
public:
    virtual ~IMessageRepository() = default;

    virtual void save(const Message &message) = 0;
    virtual QList<Message> getHistory(const QString &user1, const QString &user2, int limit = 50) = 0;
    virtual QList<Message> getMessagesForUser(const QString &username) = 0;
    virtual void markAsRead(const QString &messageId) = 0;
};

#endif