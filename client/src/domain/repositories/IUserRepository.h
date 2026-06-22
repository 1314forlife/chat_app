#ifndef IUSERREPOSITORY_H
#define IUSERREPOSITORY_H

#include <QString>
#include <QList>
#include <optional>
#include "domain/entities/User.h"

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    virtual void save(const User &user) = 0;
    virtual std::optional<User> findByUsername(const QString &username) = 0;
    virtual std::optional<User> findById(const QString &id) = 0;
    virtual QList<User> getOnlineUsers() = 0;
    virtual void setOnline(const QString &id, bool online) = 0;
    virtual QList<User> getAllUsers() = 0;
};

#endif